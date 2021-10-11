#ifndef lint
#pragma ident "@(#)soft_sp_spacetab.c 1.6 96/06/10 SMI"
#endif
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include "spmisoft_lib.h"
#include "sw_space.h"

#include <sys/fs/ufs_fs.h>
#include <sys/statvfs.h>
#include <sys/mnttab.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define	MNTTAB_FILE	"/etc/mnttab"
#define	BUCKET_INCR	32

extern	char	*slasha;
extern	int	upg_state;
#ifdef DEBUG
extern FILE	*ef;
#endif

/* Public function prototypes */

FSspace 	**get_current_fs_layout(void);
FSspace 	**load_current_fs_layout(void);
FSspace 	**load_def_spacetab(FSspace **);
FSspace 	**load_defined_spacetab(char **);
void		sort_spacetab(FSspace **);

/* Library function prototype */
FSspace		**get_master_spacetab(void);

/* Local Function Prototypes */

static	FSspace 	**alloc_spacetab(int);
static	int	do_cmp(const void *, const void *);
static	int	get_su_only(char *);
static	int	bread(daddr_t, char *, int);
static	int	getsb(struct fs *fs, char *);
static	FSspace 	**load_curmnt_spacetab(void);
static void	free_master_spacetab(void);

static FSspace	**master_spacetab = NULL;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * get_current_fs_layout()
 *	Make a copy of the current master-space table (which contains
 *	the current mounted file systems and the data that describes them).
 * Parameters:
 * Return:
 *	NULL	- couldn't allocate a FSspace pointer array 
 * Status:
 *	public
 */
FSspace **
get_current_fs_layout(void)
{
	FSspace		**sp;
	int		i, num_entries;

	if (master_spacetab == NULL)
		if ((master_spacetab = get_master_spacetab()) == NULL)
			return ((FSspace **)NULL);

	/* count the number of entries in the master space table */
	for (num_entries = 0; master_spacetab[num_entries]; num_entries++)
		;
	
	sp = alloc_spacetab(num_entries + 1);

	/* duplicate each permanent entry in the master space table */

	for (i = 0; i < num_entries; i++) {
		sp[i] = (FSspace *) xcalloc(sizeof (FSspace));
		sp[i]->fsp_mntpnt = xstrdup(master_spacetab[i]->fsp_mntpnt);
		sp[i]->fsp_pkg_databases =
		    StringListDup(master_spacetab[i]->fsp_pkg_databases);
		sp[i]->fsp_fsi = master_spacetab[i]->fsp_fsi;
	}
	sp[i] = NULL;

	return (sp);
}

/*
 * load_current_fs_layout()
 *	Clear the current master space table, if one exists.
 *	Load a space table with the UFS, "/a/??" file
 * 	systems found in the /etc/mnttab.
 * Parameters:
 * Return:
 *	NULL	- couldn't allocate a FSspace pointer array 
 * Status:
 *	public
 */
FSspace **
load_current_fs_layout(void)
{
	free_master_spacetab();
	return (get_master_spacetab());
}

/*
 * load_def_spacetab()
 *	Load a space table with default file systems. Create a new space
 *	table if one wasn't passed in. NULL the last entry as a marker.
 * Parameters:
 *	sp	- NULL if a new FSspace table should be allocated, otherwise,
 *		  a pointer to an existing space table of sufficient size.
 * Return:
 *	NULL	- table allocation failed
 *	# > 0	- pointer to FSspace table pointer array
 * Status:
 *	public
 * Note:
 *	This routine should be a void() function, pointer values being
 *	passed back through the parameter
 */
FSspace **
load_def_spacetab(FSspace **sp)
{
	int i = 0;

	if (sp == (FSspace **)NULL) {
		sp = alloc_spacetab(N_LOCAL_FS + 1);
		if (sp == (FSspace **)NULL)
			return ((FSspace **)NULL);
	}

	for (i = 0; def_mnt_pnt[i]; i++) {
		sp[i] = (FSspace *) xcalloc(sizeof (FSspace));
		sp[i]->fsp_mntpnt = xstrdup(def_mnt_pnt[i]);
	}

	sp[i] = NULL;
	sort_spacetab(sp);

	return (sp);
}

/*
 * load_defined_spacetab()
 *	Create a FSspace table pointer array, alloc out FSspace structures for
 *	each pointer, and alloc out a mountpoint pathname string for each 
 *	structure. Sort the array after loading. Return a pointer to the 
 *	shiney new structure.
 * Parameters:
 *	flist	- list of file systems which require FSspace structures
 * Return:
 *	NULL	- unable to allocate space table, or invalid parameter
 *	# > 0	- pointer to newly allocated and initialized FSspace pointer 
 * 		  array
 * Status:
 *	public
 */
FSspace **
load_defined_spacetab(char **flist)
{
	FSspace **sp;
	int 	i;

	/* parameter check */
	if (flist == (char **)NULL)
		return ((FSspace **)NULL);

	for (i = 0; flist[i]; i++) ;
	sp = alloc_spacetab(i + 1);

	if (sp == NULL)
		return ((FSspace **)NULL);

	for (i = 0; flist[i]; i++) {
		sp[i] = (FSspace *) xcalloc(sizeof (FSspace));
		sp[i]->fsp_mntpnt = xstrdup(flist[i]);
	}
	sort_spacetab(sp);
	return (sp);
}

/*
 * sort_spacetab()
 *	Sort the space table in ascending order based on the mountpoint
 *	pathnames
 * Parameters:
 *	sp 	- valid space table pointer
 * Return:
 *	none
 * Status:
 *	publice
 */
void
sort_spacetab(FSspace ** sp)
{
	int i;

	/* parameter check */
	if (sp == (FSspace **)NULL)
		return;

	for (i = 0; sp[i]; i++)
		;
	qsort((char *) sp, (size_t) i, sizeof (FSspace *), do_cmp);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
FSspace **
get_master_spacetab(void)
{
	if (master_spacetab == NULL)
		master_spacetab = load_curmnt_spacetab();
	return (master_spacetab);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
static void
free_master_spacetab(void)
{
	int	i;

	if (master_spacetab != NULL) {
		for (i = 0; master_spacetab[i]; i++) {
			free(master_spacetab[i]->fsp_fsi->fsi_device);
			free(master_spacetab[i]->fsp_fsi);
		}
		free_space_tab(master_spacetab);
		master_spacetab = NULL;
	}
}

/*
 * Lifted from tunefs.c
 */
static int	fi;
static union {
	struct fs sb;
	char pad[SBSIZE];
} sbun;
#define	sblock sbun.sb

static int
get_su_only(char * special)
{
	if (getsb(&sblock, special) < 0)
		return (10);

	return (sblock.fs_minfree);
}

/*
 * getsb()
 *	Retrieve the superblock from raw device 'file'
 * Parameters:
 *	fs	-
 *	file	-
 * Return:
 *	 0	-
 *	-1	-
 * Status:
 *	private
 */
static int
getsb(struct fs * fs, char * file)
{
	fi = open(file, O_RDONLY);
	if (fi < 0)
		return (-1);

	if (bread(SBLOCK, (char *)fs, SBSIZE)) {
		(void) close (fi);
		return (-1);
	}

	(void) close (fi);

	if (fs->fs_magic != FS_MAGIC)
		return (-1);

	return (0);
}

/*
 * bread()
 * Parameters:
 *	bno	- block number
 *	buf	- buffer with which to retrieve data
 *	cnt	- size of 'buf'
 * Return:
 *	0	-
 *	1	-
 * Status:
 *	private
 */
static int
bread(daddr_t bno, char * buf, int cnt)
{
	int	i;
	int	pos;

	if ((pos = llseek(fi, (offset_t)bno * DEV_BSIZE, 0)) < 0)
		return (1);

	if ((i = read(fi, buf, cnt)) != cnt)
		return (1);

	i++;
	pos++;
	return (0);
}

/*
 * do_cmp()
 *	Comparitor function for qsort() call in sort_spacetab()
 * Parameters:
 *	v1	- first FSspace table entry
 *	v2	- second entry
 * Return:
 *	 0	- strings are identical
 *	 1	- s2 > s1
 *	-1	- s1 > s2
 * Status:
 *	private
 */
static int
do_cmp(const void *v1, const void *v2)
{
	char *s1;
	char *s2;

	FSspace **sp1 = (FSspace **)v1;
	FSspace **sp2 = (FSspace **)v2;

	s1 = (*sp1)->fsp_mntpnt;
	s2 = (*sp2)->fsp_mntpnt;

	/*
	 *  If either are null, just return 1.  The answer doesn't
	 *  matter, since the one that was NULL had better have its
 	 *  FS_IGNORE_ENTRY flag set.  The important thing is to avoid
	 *  calling strcoll with a null argument.
	 */
	if (s1 == NULL || s2 == NULL)
		return (1);
	return ((-1) * strcoll(s1, s2));
}

/*
 * load_curmnt_spacetab()
 *	Load the space table which describes the current, actual
 *	state of the system.
 * Parameters:
 *	sp	- FSspace structure pointer
 * Return:
 *	NULL	- couldn't allocate a FSspace pointer array 
 * Status:
 *	public
 * Note:
 *	alloc routine
 */
static FSspace **
load_curmnt_spacetab(void)
{
	FSspace		**sp;
	register int	j;
	Fsinfo		*fsp;
	int		ret, i, num_buckets;
	char		*cp;
	FILE		*fp = (FILE *) NULL;
	struct	mnttab 	mp;
	struct	statvfs	svfsbuf;
	struct	stat	sbuf;
	FSspace 		**lsp;

	num_buckets = BUCKET_INCR;
	sp = alloc_spacetab(BUCKET_INCR);
	if (sp == (FSspace **)NULL) {
		set_sp_err(SP_ERR_MALLOC, 0, NULL);
		return ((FSspace **)NULL);
	}

	/* read the mounttab file and scan for all mounted UFS files systems */
	if ((fp = fopen(MNTTAB_FILE, "r")) == (FILE *) NULL) {
		set_sp_err(SP_ERR_OPEN, errno, MNTTAB_FILE);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: get_current_fs_layout(): "
		    "Can't open /etc/mnttab\n");
#endif
		return (NULL);
	}

	/* find all mounted filesystems of type ufs */
	ret = i = 0;
	while (ret == 0) {

		if ((ret = getmntent(fp, &mp)) != 0) {
			if (ret == -1) {			/* EOF */
				sp[i] = (void *)NULL;
				continue;
			} else {				/* error */
				set_sp_err(SP_ERR_GETMNTENT, ret, NULL);
#ifdef DEBUG
				(void) fprintf(ef,
				    "DEBUG: get_current_fs_layout():\n");
				perror("getmntent");
#endif
				fclose(fp);
				return ((FSspace **)NULL);
			}
		}

		/* Filter entries which are not type "ufs" and those not mounted 
		 * under slasha.
		 */
		if (strcmp(mp.mnt_fstype, "ufs") != 0)
			continue;

		if (slasha) {
			if (strncmp(mp.mnt_mountp, slasha, strlen(slasha)) != 0)
				continue;
		}


		/* allocate a FSspace structure, a Fsinfo structure, and
		 * initialize the FS device, real mount path name, device ID,
		 * and all the file system size info from statvfs() 
		 */
		sp[i] = (FSspace *) xcalloc(sizeof (FSspace));

		fsp = (Fsinfo *) xcalloc(sizeof (Fsinfo));

		sp[i]->fsp_fsi = fsp;
		fsp->fsi_device = xstrdup(mp.mnt_special);

		if (slasha) {
			/* If "/a" then "/" else strip /a prefix */
			cp = mp.mnt_mountp + strlen(slasha);
			if (*cp == '\0')
				sp[i]->fsp_mntpnt = xstrdup("/");
			else
				sp[i]->fsp_mntpnt = xstrdup(cp);
		} else
			sp[i]->fsp_mntpnt = xstrdup(mp.mnt_mountp);

		if (stat(mp.mnt_mountp, &sbuf) < 0) {
			set_sp_err(SP_ERR_STAT, errno, mp.mnt_mountp);
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: get_current_fs_layout(): "
			    "stat failed for %s\n",
				mp.mnt_mountp);
			perror("stat");
#endif
			fclose(fp);
			return ((FSspace **)NULL);
		} else
			fsp->f_st_dev = sbuf.st_dev;

		if (statvfs(mp.mnt_mountp, &svfsbuf) < 0) {
			set_sp_err(SP_ERR_STATVFS, errno, mp.mnt_mountp);
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: get_current_fs_layout():\n");
			perror("statvfs");
#endif
			fclose(fp);
			return (NULL);
		} else {
			fsp->f_frsize = svfsbuf.f_frsize;
			fsp->f_blocks = svfsbuf.f_blocks;
			fsp->f_bfree = svfsbuf.f_bfree;
			fsp->f_files = svfsbuf.f_files;
			fsp->f_ffree = svfsbuf.f_ffree;
			fsp->f_bavail = svfsbuf.f_bavail;
		}
		/*
		 * load the reserved SU block count (or set it if not an
		 * upgrade
		 */
		if (upg_state & SP_UPG)
			fsp->su_only = get_su_only(mp.mnt_special);
		else
			fsp->su_only = 10;

		if (fsp->su_only >= 100)  /* 100 is the limit in this code */
			fsp->su_only = 10;
		/*
		 * if there weren't enough buckets in the original request, go
		 * add some more to the space array 
		 */
		if (++i == num_buckets) {
			num_buckets += BUCKET_INCR;
			lsp = (FSspace **)xrealloc((void *)sp,
			    (sizeof (FSspace *) * num_buckets));
			sp = lsp;

			for (j = i; j < num_buckets; j++)
				sp[j] = NULL;
		}
	}
	fclose(fp);

	if (sp[0] == NULL) {
		set_sp_err(SP_ERR_NOSLICES, 0, NULL);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: get_current_fs_layout(): "
		    "No slices found !\n");
#endif
		return ((FSspace **)NULL);
	}

	sort_spacetab(sp);
	return (sp);
}

/*
 * alloc_spacetab()
 *	Create a FSspace table pointer array of size 'buckets' out of the heap.
 *	This does not allocate the FSspace structures, but only the array of
 *	structure pointers.
 * Parameters:
 *	buckets	- number of FSspace structures in the array
 * Return:
 *	NULL	- invalid parameter or alloc failure
 *	# > 0	- pointer to newly allocated FSspace table structure
 * Status:
 *	private
 * Note:
 *	alloc routine
 */
static FSspace **
alloc_spacetab(int buckets)
{
	/* parameter check */
	if (buckets <= 0)
		return ((FSspace **)NULL);
	else
		return ((FSspace **) xcalloc(buckets * sizeof (FSspace *)));
}
