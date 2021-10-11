#ifndef lint
#pragma ident "@(#)soft_sp_util.c 1.7 96/06/03 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#include "spmisoft_lib.h"
#include "sw_space.h"

#include <sys/statvfs.h>
#include <stdarg.h>
#include <memory.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/* Public Function Prototypes */

int		swi_valid_mountp(char *);
void		fsp_add_to_field(FSspace *, FSPfield, long);
void		fsp_set_field(FSspace *, FSPfield, long);

/* Library function prototypes */
int		do_chroot(char *);
int		check_path_for_vars(char *);
int		valid_mountp_list(char **);
void		set_path(char *, char *, char *, char *);
void		reset_stab(FSspace **);
int		meets_reqs(Modinfo *);
ContentsRecord	*contents_record_from_stab(FSspace **, ContentsRecord *);
void		stab_from_contents_record(FSspace **, ContentsRecord *);

/* internal function prototype */
static void	sync_fsp_values(FSspace *);
static int	map_mntpnt_to_cridx(char *);
static char	*map_cridx_to_mntpnt(int);
static ContentsRecord *cr_match_idx(ContentsRecord *, int);

extern	char 	*slasha;
#ifdef DEBUG
char 		*root_path;
extern FILE	*ef;
#endif

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * valid_mountp()
 *	Boolean check if the mount point is a string starting with a '/'.
 * Parameters:
 *	mountp	- pathname for mount point
 * Return:
 *	0 	- failure; invalid pathname
 *	1 	- success
 * Status:
 *	public
 */
int
swi_valid_mountp(char * mountp)
{
	if (mountp == (char *)NULL || *mountp != '/')
		return (0);
	else
		return (1);
}

/*
 * valid_mountp_list()
 *	Boolean function to check an array of pathnames and verify if they are
 *	valid fully qualified pathnames. The only exception permitted is the 
 *	partition identifier "swap".  "/" must be in the list. There is no 
 *	checking for duplication or any testing if the pathnames actually exist.
 * Parameters:
 *	mountp_list	- an array of strings containing mountpoint paths 
 *			  (or "swap"). The array may contain 0 or more valid 
 *			  entries, but cannot be a NULL array itself. At least 
 *			  one of the strings must be "/", or the list is invalid
 *			  (minima test).
 * Returns:
 *	0 	- failure: invalid mount point list
 *	1 	- valid mount points
 * Status:
 *	public
 */
int
valid_mountp_list(char ** mp_list)
{
	int	i;
	int	found_slash = 0;

	if (mp_list != (char **)NULL) {
		for (i = 0; mp_list[i]; i++) {
			if (strcmp(mp_list[i], "/") == 0)
				found_slash = 1;
			if (*mp_list[i] != '/' &&
					strcmp (mp_list[i], "swap") != 0)
				return(0);
		}
	}
	if (!found_slash)
		return (0);
	else
		return (1);
}

void
fsp_add_to_field(FSspace *stab, FSPfield field, long val)
{
	switch (field)
	{
	case (FSP_CONTENTS_PKGD):
		stab->fsp_cts.contents_packaged += val;
		break;
	case (FSP_CONTENTS_NONPKG):
		stab->fsp_cts.contents_nonpkg += val;
		break;
	case (FSP_CONTENTS_DEVFS):
		stab->fsp_cts.contents_devfs += val;
		break;
	case (FSP_CONTENTS_SAVEDFILES):
		stab->fsp_cts.contents_savedfiles += val;
		break;
	case (FSP_CONTENTS_PKG_OVHD):
		stab->fsp_cts.contents_pkg_ovhd += val;
		break;
	case (FSP_CONTENTS_PATCH_OVHD):
		stab->fsp_cts.contents_patch_ovhd += val;
		break;
	case (FSP_CONTENTS_SU_ONLY):
		stab->fsp_su_only += val;
		break;
	case (FSP_CONTENTS_REQD_FREE):
		stab->fsp_reqd_free += val;
		break;
	case (FSP_CONTENTS_UFS_OVHD):
		stab->fsp_ufs_ovhd += val;
		break;
	case (FSP_CONTENTS_ERR_EXTRA):
		stab->fsp_err_extra += val;
		break;
	}

	sync_fsp_values(stab);
}

void
fsp_set_field(FSspace *stab, FSPfield field, long val)
{
	switch (field)
	{
	case (FSP_CONTENTS_PKGD):
		stab->fsp_cts.contents_packaged = val;
		break;
	case (FSP_CONTENTS_NONPKG):
		stab->fsp_cts.contents_nonpkg = val;
		break;
	case (FSP_CONTENTS_DEVFS):
		stab->fsp_cts.contents_devfs = val;
		break;
	case (FSP_CONTENTS_SAVEDFILES):
		stab->fsp_cts.contents_savedfiles = val;
		break;
	case (FSP_CONTENTS_PKG_OVHD):
		stab->fsp_cts.contents_pkg_ovhd = val;
		break;
	case (FSP_CONTENTS_PATCH_OVHD):
		stab->fsp_cts.contents_patch_ovhd = val;
		break;
	case (FSP_CONTENTS_SU_ONLY):
		stab->fsp_su_only = val;
		break;
	case (FSP_CONTENTS_REQD_FREE):
		stab->fsp_reqd_free = val;
		break;
	case (FSP_CONTENTS_UFS_OVHD):
		stab->fsp_ufs_ovhd = val;
		break;
	case (FSP_CONTENTS_ERR_EXTRA):
		stab->fsp_err_extra = val;
		break;
	}

	sync_fsp_values(stab);
}


/*
 * do_chroot() 
 *	Change root to path and chdir to the new root.  The firstime this is
 *	called get a fd for the real root so we can use fchroot() to return
 *	there.
 * Parameters:
 *	path 	- pathname for chroot
 * Return:
 *	0 	- chroot failed
 *	1 	- chroot succeeded
 * Status:
 *	public
 */
int
do_chroot(char * path)
{
	static int	root_fd = -1;

	/* parameter check */
	if (path == (char *)NULL || *path == '\0')
		return(0);

	if (root_fd < 0) {
		if ((root_fd = open("/", O_RDONLY)) < 0) {
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: do_chroot(): '/':\n");
			perror("open");
#endif
			return (0);
		}
	}

	if (strcmp(path, "/") == 0) {
		if (fchroot(root_fd) < 0) {
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: do_chroot():\n");
			perror("fchroot");
#endif
			return (0);
		}
	} else if (chroot(path) < 0)
		return (0);

	return (1);
}

/*
 * check_path_for_vars()
 *	Determines if the pathname contains the variable pattern "/$" or "^$"
 *	 (where '^' means beginning of string).
 * Paramaters:
 *	path	- path to check
 * Return:
 *	0 	- no variable pattern found
 *	1 	- variable pattern found
 * Status:
 *	public
 */
int
check_path_for_vars(char * path)
{
	char	*cp;

	if (path == (char *)NULL || *path == '\0')
		return (0);

	if ((cp = strchr(path, '$')) != (char *)NULL) {
		if (cp == path || (*(cp-1) == '/'))
			return (1);
	}

	return (0);
}

/*
 * set_path()
 *	Concatenate 'rootdir', 'basedir' (or instdir) and a 'path', and create
 *	a form without leading "//"s or a trailing "/"s.  
 * Parameters:
 *	pathbuf		- return the concatenated string here
 * 	rootdir_p	- first component of path
 *	basedir_p	- second component of path
 *	path		- pathname to file/dir
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_path(char *pathbuf, char *rootdir_p, char *basedir_p, char *path)
{
	char	*slash = "/";
	char	*empty_str = "";
	char	*rootdir;
	char	*basedir;

	if (rootdir_p == NULL)
		rootdir_p = slash;
	if (basedir_p == NULL)
		basedir_p = slash;
#ifdef DEBUG
	if (*rootdir_p != '/') {
		(void) fprintf(ef, "DEBUG: set_path():\n");
		(void) fprintf(ef, "rootdir_p %s not absolute.\n", rootdir_p);
	}
	if (*basedir_p != '/') {
		(void) fprintf(ef, "DEBUG: set_path():\n");
		(void) fprintf(ef, "basedir_p %s not absolute.\n", basedir_p);
	}
#endif

	if (strlen(rootdir_p) == 1)
		rootdir = empty_str;
	else
		rootdir = rootdir_p;

	if (strlen(basedir_p) == 1)	
		basedir = empty_str;
	else
		basedir = basedir_p;

	while (*(rootdir + 1) == '/')
		rootdir++;
	while (*(basedir + 1) == '/')	
		basedir++;

	if (*path != '/')
		(void) sprintf(pathbuf, "%s%s/%s", rootdir, basedir, path);
	else
		(void) sprintf(pathbuf, "%s%s%s", rootdir, basedir, path);

	if (strlen(pathbuf) > (size_t) 1) {
		if (pathbuf[strlen(pathbuf) - 1] == '/') {
			pathbuf[strlen(pathbuf) - 1] = '\0';
		}
	}
}

/*
 * reset_stab()
 *	Reset the FSspace blocks and inodes (files) fields to 0 for all space
 *	structures passed in.
 * Parameters:
 *	sp	- valid FSspace table pointer
 * Return:
 *	none
 * Status:
 *	public
 */
void
reset_stab(FSspace **sp)
{
	int i;

	if (sp != (FSspace **)NULL) {
		for (i = 0; sp[i]; i++) {
			sp[i]->fsp_flags &= ~FS_LIBRARY_FLAGS_MASK;
			sp[i]->fsp_reqd_slice_size = 0;
			sp[i]->fsp_reqd_contents_space = 0;
			fsp_set_field(sp[i], FSP_CONTENTS_PKGD, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_NONPKG, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_DEVFS, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_SAVEDFILES, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_PKG_OVHD, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_PATCH_OVHD, 0);
			sp[i]->fsp_cts.contents_inodes_used = 0;
			fsp_set_field(sp[i], FSP_CONTENTS_SU_ONLY, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_REQD_FREE, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_UFS_OVHD, 0);
			fsp_set_field(sp[i], FSP_CONTENTS_ERR_EXTRA, 0);
		}
	}
	return;
}

/*
 * meets_reqs()
 *	Test whether this module is one that needs to be loaded.
 * Parameters:
 *	mi	- modifo structure pointer 
 * Return:
 *	1 	- valid module
 *	0 	- Error: invalid module or Modinfo pointer
 * Status:
 *	public
 */
int
meets_reqs(Modinfo *mi)
{
	/* parameter check */
	if (mi == NULL)
		return (0);

	if ((mi->m_shared == NOTDUPLICATE) ||
	    (mi->m_shared == SPOOLED_NOTDUP)) {
		if ((mi->m_status == SELECTED) ||
		    (mi->m_status == REQUIRED)) {
			if ((mi->m_action == TO_BE_PKGADDED) ||
			    (mi->m_action == TO_BE_SPOOLED)) {
				return (1);
			}
		} else if ((mi->m_status == LOADED) &&
			    (mi->m_action == TO_BE_PRESERVED)) {
			return (1);
		}
	}

	return (0);
}

ContentsRecord *
contents_record_from_stab(FSspace **fstab, ContentsRecord *cr_in)
{
	int	i, idx;
	ContentsRecord	*crhead, *cr;

	if (cr_in == NULL)
		crhead = NULL;
	else {
		crhead = cr_in;
		for (cr = crhead; cr; cr = cr->next) {
			cr->ctsrec_brkdn.contents_packaged = 0;
			cr->ctsrec_brkdn.contents_nonpkg = 0;
			cr->ctsrec_brkdn.contents_devfs = 0;
			cr->ctsrec_brkdn.contents_savedfiles = 0;
			cr->ctsrec_brkdn.contents_pkg_ovhd = 0;
			cr->ctsrec_brkdn.contents_patch_ovhd = 0;
			cr->ctsrec_brkdn.contents_inodes_used = 0;
		}
	}
	for (i = 0; fstab[i]; i++) {
		if (fstab[i]->fsp_reqd_contents_space != 0 ||
		    fstab[i]->fsp_cts.contents_inodes_used != 0) {
			if (fstab[i]->fsp_flags & FS_IGNORE_ENTRY)
				continue;
			idx = map_mntpnt_to_cridx(fstab[i]->fsp_mntpnt);
			cr = cr_match_idx(crhead, idx);
			if (cr == NULL) {
				cr = (ContentsRecord *)xcalloc((size_t)
				    sizeof(ContentsRecord));
				cr->ctsrec_idx = idx;
				link_to((Item **)&crhead, (Item *)cr);
			}
			cr->ctsrec_brkdn.contents_packaged =
			    fstab[i]->fsp_cts.contents_packaged;
			cr->ctsrec_brkdn.contents_nonpkg = 
			    fstab[i]->fsp_cts.contents_nonpkg;
			cr->ctsrec_brkdn.contents_devfs =
			    fstab[i]->fsp_cts.contents_devfs;
			cr->ctsrec_brkdn.contents_savedfiles =
			    fstab[i]->fsp_cts.contents_savedfiles;
			cr->ctsrec_brkdn.contents_pkg_ovhd =
			    fstab[i]->fsp_cts.contents_pkg_ovhd;
			cr->ctsrec_brkdn.contents_patch_ovhd =
			    fstab[i]->fsp_cts.contents_patch_ovhd;
			cr->ctsrec_brkdn.contents_inodes_used =
			    fstab[i]->fsp_cts.contents_inodes_used;
		}
	}
	return (crhead);
}

void
stab_from_contents_record(FSspace **fsp, ContentsRecord *cr)
{
	char	*mntpnt;
	int	i;

	reset_stab(fsp);
	for ( ; cr != NULL; cr = cr->next) {
		mntpnt = map_cridx_to_mntpnt(cr->ctsrec_idx);
		for (i = 0; fsp[i] != NULL; i++) {
			if (streq(fsp[i]->fsp_mntpnt, mntpnt)) {
				fsp_set_field(fsp[i], FSP_CONTENTS_PKGD,
				    cr->ctsrec_brkdn.contents_packaged);
				fsp_set_field(fsp[i], FSP_CONTENTS_NONPKG,
				    cr->ctsrec_brkdn.contents_nonpkg);
				fsp_set_field(fsp[i], FSP_CONTENTS_DEVFS,
				    cr->ctsrec_brkdn.contents_devfs);
				fsp_set_field(fsp[i], FSP_CONTENTS_SAVEDFILES,
				    cr->ctsrec_brkdn.contents_savedfiles);
				fsp_set_field(fsp[i], FSP_CONTENTS_PKG_OVHD,
				    cr->ctsrec_brkdn.contents_pkg_ovhd);
				fsp_set_field(fsp[i], FSP_CONTENTS_PATCH_OVHD,
				    cr->ctsrec_brkdn.contents_patch_ovhd);
				fsp[i]->fsp_cts.contents_inodes_used =
				    cr->ctsrec_brkdn.contents_inodes_used;
				break;
			}
		}
	}
}

static void
sync_fsp_values(FSspace *stab)
{
	stab->fsp_reqd_contents_space =
	    stab->fsp_cts.contents_packaged + 
	    stab->fsp_cts.contents_nonpkg + 
	    stab->fsp_cts.contents_devfs + 
	    stab->fsp_cts.contents_savedfiles + 
	    stab->fsp_cts.contents_pkg_ovhd + 
	    stab->fsp_cts.contents_patch_ovhd;

	stab->fsp_reqd_slice_size =
	    stab->fsp_reqd_contents_space +
	    stab->fsp_su_only +
	    stab->fsp_reqd_free +
	    stab->fsp_ufs_ovhd +
	    stab->fsp_err_extra;

	/*
	 *  If the total required contents space includes anything
 	 *  other than packaged or package-related data, mark it as
	 *  having "PACKAGED" data.
	 */
	if (stab->fsp_reqd_contents_space > stab->fsp_cts.contents_nonpkg)
		stab->fsp_flags |= FS_HAS_PACKAGED_DATA;
}

static int
map_mntpnt_to_cridx(char *mntpnt)
{
	FSspace **msp;
	int	i;

	msp = get_master_spacetab();
	if (msp == NULL)
		return (-1);
	for (i = 0; msp[i] != NULL; i++)
		if (streq(mntpnt, msp[i]->fsp_mntpnt))
			return (i);
	return (-1);
}

static char *
map_cridx_to_mntpnt(int idx)
{
	FSspace	**msp;

	msp = get_master_spacetab();
	if (msp == NULL)
		return (NULL);
	return (msp[idx]->fsp_mntpnt);
}

static ContentsRecord *
cr_match_idx(ContentsRecord *crhead, int idx)
{
	ContentsRecord	*cr;

	for (cr = crhead; cr != NULL; cr = cr->next)
		if (cr->ctsrec_idx == idx)
			return (cr);
	return (NULL);
}
