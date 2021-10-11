#ifndef lint
#pragma ident   "@(#)sp_util.c 1.24 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "sw_lib.h"
#include "sw_space.h"

#include <sys/statvfs.h>
#include <stdarg.h>
#include <memory.h>
#include <fcntl.h>

/* Public Function Prototypes */

int		swi_valid_mountp(char *);

/* Library function prototypes */
int		do_chroot(char *);
int		check_path_for_vars(char *);
int		valid_mountp_list(char **);
void		set_path(char *, char *, char *, char *);
void		reset_stab(Space **);
int		reset_swm_stab(Space **);
int		meets_reqs(Modinfo *);
daddr_t		tot_bavail(Space **, int);
u_long		new_slice_size(u_long, int);

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
 *	Reset the Space blocks and inodes (files) fields to 0 for all space
 *	structures passed in.
 * Parameters:
 *	sp	- valid Space table pointer
 * Return:
 *	none
 * Status:
 *	public
 */
void
reset_stab(Space **sp)
{
	int i;

	if (sp != (Space **)NULL) {
		for (i = 0; sp[i]; i++) {
			sp[i]->bused = 0;
			sp[i]->fused = 0;
		}
	}
	return;
}

/*
 * reset_swm_stab()
 *	Reload the SWM space table with actual space data from statvfs() for
 *	each file system in the table. Reset space blocks and (indoes) files
 *	used to 0. Note that the mount point is relative to "/a". All mount
 *	paths which are not absolute are ignored.
 * Parameters:
 *	sp	- valid space table pointer
 * Return:
 *	SP_ERR_PARAM_INVAL - NULL Space prray pointer, or NULL Fsinfo
 *			     pointer encountered
 *	SP_FAILURE	   - couldn't stat one of the mountpoints
 *	SUCCESS		   - table reset successfully
 * Status:
 *	public
 */
int
reset_swm_stab(Space **sp)
{
	int	i;
	char	mpath[MAXPATHLEN];
	struct  statvfs svfsbuf;

	/* parameter check */
	if (sp == (Space **)NULL)
		return (SP_ERR_PARAM_INVAL);

	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsi == NULL)
			return (SP_ERR_PARAM_INVAL);

		if (*sp[i]->mountp != '/')
			continue;

		sp[i]->bused = 0;
		sp[i]->fused = 0;

		set_path(mpath, slasha, NULL, sp[i]->mountp);
		if (statvfs(mpath, &svfsbuf) < 0) {
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: reset_swm_stab(): %s\n", 	
				sp[i]->mountp);
			perror("statvfs");
#endif
			return (SP_FAILURE);
		}
		sp[i]->fsi->f_bfree = svfsbuf.f_bfree;
		sp[i]->fsi->f_bavail = svfsbuf.f_bavail;
		sp[i]->fsi->f_ffree = svfsbuf.f_ffree;
		sp[i]->fsi->f_files = svfsbuf.f_files;
	}

	return (SUCCESS);
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

/*
 * tot_bavail()
 *	Calculate the total blocks available on the file system, less the
 *	number of blocks reserved for su-only use.
 * Parameters:
 *	sp	- space table pointer
 *	i	- index for file system in space table >= 0
 * Return:
 *	# > 0 	- number of blocks tabulated
 *	0	- parameter invalid, or su_only value exceeds legal value
 * Status:
 *	public
 */
daddr_t
tot_bavail(Space **sp, int i)
{
	Fsinfo 	*fp;

	/* parameter check */
	if (sp == (Space **)NULL || i < 0 || sp[i]->fsi == (Fsinfo *)NULL)
		return(0);

	fp = sp[i]->fsi;
	if (fp->su_only >= 100)
		return(0);
	else
		return (((100 - fp->su_only) * fp->f_blocks)/100);
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
 *		 free ona particular file system, expressed as a percentage
 *		 (i.e., 10% free space means fsfree = 10)
 *	su_only = percent of file system reserved for root-only access,
 *		 expressed as a percentage (10% free means su_only = 10).
 *	ufs_oh = percent of a disk slice that is used for UFS overhead,
 *		 expressed as a percentage (10% free means ufs_oh = 10. 
 *		 This is actually difficult to predict, since the percentage
 *		 varies according to the size of the file system (larger
 *		 file systems use a lower percentage for ufs overhead).
 *		 We assume a worst-case here, which is about 10%.
 *	nblks =  The total number of 1024 blocks in a disk slice.
 *	needed = space required for actual contents (not including required
 *		 free space), expressed in 1024 blocks.
 *	
 *	The following condition must be satisfied:
 *	
 *	needed <= nblks * (100 - ufs_oh)(100 - su_only)(100 - fsfree)
 *			   -----------   -------------  ------------
 *			       100           100	    100
 *	
 *	Solving for nblks, we get:
 *
 *	nblks >=                       needed
 *		    --------------------------------------------------
 *			(100 - ufs_oh)(100 - su_only)(100 - fsfree)
 *			 -----------   -------------  ------------
 *			      100           100	           100
 *		
 *	Rearranging this a bit:
 *	
 *	nblks >=                needed *      100     
 *					  ------------
 *					 (100 - fs_free)
 *		    --------------------------------------------------
 *			       (100 - ufs_oh)(100 - su_only)
 *			        -----------   -------------
 *			             100           100
 *		
 *	When this function is called, the value "size" is equal to
 *	the numerator of the right-hand size of the equation.  Since
 *	ufs_oh is a constant with value = 10, the (100 - ufs_oh)/100
 *	becomes .9, which is what ufs_ovhd_fraction is set to below.
 *
 * Return:
 *	# >= 0  - # of blocks generally available in new size
 * Status:
 *	public
 */
u_long
new_slice_size(u_long size, int su_only)
{
	u_long	new_size;
	float	float_new_size, ufs_ovhd_fraction, su_only_fraction;

	/* parameter check */
	if (su_only >= 100)
		return (0);
	else {	
		ufs_ovhd_fraction = 0.9;

		su_only_fraction = (100.0 - (float)su_only)/100.0;

		float_new_size = (float)size /
		    (ufs_ovhd_fraction * su_only_fraction);

		new_size = float_new_size;  /* convert to u_long */

		/* round up if fractional part was truncated. */
		if (new_size != float_new_size)
			new_size++;

		return (new_size);
	}
}
