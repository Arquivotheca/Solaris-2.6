#ifndef lint
#pragma ident   "@(#)sp_spacetab.c 1.23 95/05/31 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "sw_lib.h"
#include "sw_space.h"

#include <sys/fs/ufs_fs.h>
#include <sys/statvfs.h>
#include <sys/mnttab.h>
#include <fcntl.h>
#include <errno.h>

#define	MNTTAB_FILE	"/etc/mnttab"
#define	BUCKET_INCR	32

extern	char	*slasha;
extern	int	upg_state;
#ifdef DEBUG
extern FILE	*ef;
#endif

/* Public function prototypes */

Space 		**load_spacetab(Space **, char **);
Space 		**load_def_spacetab(Space **);
Space 		**load_defined_spacetab(char **);
void		sort_spacetab(Space **);
void		zero_spacetab(Space **);

/* Local Function Prototypes */

static	Space 	**alloc_spacetab(int);
static	int	do_cmp(const void *, const void *);
static	int	get_su_only(char *);
static	int	bread(daddr_t, char *, int);
static	int	getsb(struct fs *fs, char *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * load_spacetab()
 *	Load a space table (new or passed in) with the UFS, "/a/??" file
 * 	systems found in the /etc/mnttab. Loading can be restricted by the
 *	'flist' list of mount points (not prefixed with "/a").
 * Parameters:
 *	sp	- Space structure pointer
 *	flist	- list of file systems to which the load should be
 *		  restricted
 * Return:
 *	NULL	- couldn't allocate a Space pointer array 
 * Status:
 *	public
 * Note:
 *	alloc routine
 */
Space **
load_spacetab(Space ** sp, char ** flist)
{
	register int	j;
	Fsinfo		*fsp;
	int		ret, i, num_buckets;
	char		*cp;
	char		*slash = "/";
	FILE		*fp = (FILE *) NULL;
	struct	mnttab 	mp;
	struct	statvfs	svfsbuf;
	struct	stat	sbuf;
	Space 		**lsp;

	/* allocate a Space pointer array if none was provided */
	if (sp == NULL) {
		num_buckets = BUCKET_INCR;
		sp = alloc_spacetab(BUCKET_INCR);
		if (sp == (Space **)NULL) {
			set_sp_err(SP_ERR_MALLOC, 0, NULL);
			return ((Space **)NULL);
		}
	}

	/* read the mounttab file and scan for all mounted UFS files systems */
	if ((fp = fopen(MNTTAB_FILE, "r")) == (FILE *) NULL) {
		set_sp_err(SP_ERR_OPEN, errno, MNTTAB_FILE);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: load_spacetab(): Can't open /etc/mnttab\n");
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
				(void) fprintf(ef, "DEBUG: load_spacetab():\n");
				perror("getmntent");
#endif
				fclose(fp);
				return ((Space **)NULL);
			}
		}

		/* Filter entries which are not type "ufs" and those not mounted 
		 * under slasha. If flist is not NULL then further exclude the 
		 * candidates.
		 */
		if (strcmp(mp.mnt_fstype, "ufs") != 0)
			continue;

		if (slasha) {
			if (strncmp(mp.mnt_mountp, slasha, strlen(slasha)) != 0)
				continue;
		}

		if (flist) {
			if (slasha) {
				cp = mp.mnt_mountp + strlen(slasha);
				if (*cp == '\0')
					cp = slash;
			} else 	
				cp = mp.mnt_mountp;

			for (j = 0 ; flist[j] ; j++) {
				if (strcoll(cp, flist[j]) == 0) {
					j = -1;
					break;
				}
			}
			
			if (j > 0) 		/* flist exists, but no match */
				continue;
		}

		/* allocate a Space structure, a Fsinfo structure, and initialize 
		 * the FS device, real mount path name, device ID, and all the file
		 * system size info from statvfs() 
		 */
		sp[i] = (Space *) xcalloc(sizeof (Space));

		fsp = (Fsinfo *) xcalloc(sizeof (Fsinfo));

		sp[i]->fsi = fsp;
		fsp->device = strdup(mp.mnt_special);

		if (slasha) {
			/* If "/a" then "/" else strip /a prefix */
			cp = mp.mnt_mountp + strlen(slasha);
			if (*cp == '\0')
				sp[i]->mountp = strdup("/");
			else
				sp[i]->mountp = strdup(cp);
		} else
			sp[i]->mountp = strdup(mp.mnt_mountp);

		if (stat(mp.mnt_mountp, &sbuf) < 0) {
			set_sp_err(SP_ERR_STAT, errno, mp.mnt_mountp);
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: load_spacetab(): stat failed for %s\n",
				mp.mnt_mountp);
			perror("stat");
#endif
			fclose(fp);
			return ((Space **)NULL);
		} else
			fsp->st_dev = sbuf.st_dev;

		if (statvfs(mp.mnt_mountp, &svfsbuf) < 0) {
			set_sp_err(SP_ERR_STATVFS, errno, mp.mnt_mountp);
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: load_spacetab():\n");
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
		if (upg_state)
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
			/*
			 * extra step in case of xrealloc failure, so the
			 * original structure won't be lost if it was passed
			 * in as a parameter
			 */
			lsp = (Space **)xrealloc((void *)sp, (sizeof (Space *) * num_buckets));
			sp = lsp;

			for (j = i; j < num_buckets; j++)
				sp[j] = NULL;
		}
	}
	fclose(fp);

	if (sp[0] == NULL) {
		set_sp_err(SP_ERR_NOSLICES, 0, NULL);
#ifdef DEBUG
		(void) fprintf(ef,
				"DEBUG: load_spacetab(): No slices found !\n");
#endif
		return ((Space **)NULL);
	}

	sort_spacetab(sp);
	return (sp);
}

/*
 * load_def_spacetab()
 *	Load a space table with default file systems. Create a new space
 *	table if one wasn't passed in. NULL the last entry as a marker.
 * Parameters:
 *	sp	- NULL if a new Space table should be allocated, otherwise,
 *		  a pointer to an existing space table of sufficient size.
 * Return:
 *	NULL	- table allocation failed
 *	# > 0	- pointer to Space table pointer array
 * Status:
 *	public
 * Note:
 *	This routine should be a void() function, pointer values being
 *	passed back through the parameter
 */
Space **
load_def_spacetab(Space **sp)
{
	int i = 0;

	if (sp == (Space **)NULL) {
		sp = alloc_spacetab(N_LOCAL_FS + 1);
		if (sp == (Space **)NULL)
			return ((Space **)NULL);
	}

	for (i = 0; def_mnt_pnt[i]; i++) {
		sp[i] = (Space *) xcalloc(sizeof (Space));
		sp[i]->mountp = strdup(def_mnt_pnt[i]);
	}

	sp[i] = NULL;
	sort_spacetab(sp);

	return (sp);
}

/*
 * load_defined_spacetab()
 *	Create a Space table pointer array, alloc out Space structures for
 *	each pointer, and alloc out a mountpoint pathname string for each 
 *	structure. Sort the array after loading. Return a pointer to the 
 *	shiney new structure.
 * Parameters:
 *	flist	- list of file systems which require Space structures
 * Return:
 *	NULL	- unable to allocate space table, or invalid parameter
 *	# > 0	- pointer to newly allocated and initialized Space pointer 
 * 		  array
 * Status:
 *	public
 */
Space **
load_defined_spacetab(char **flist)
{
	Space **sp;
	int 	i;

	/* parameter check */
	if (flist == (char **)NULL)
		return ((Space **)NULL);

	for (i = 0; flist[i]; i++) ;
	sp = alloc_spacetab(i + 1);

	if (sp == NULL)
		return ((Space **)NULL);

	for (i = 0; flist[i]; i++) {
		sp[i] = (Space *) xcalloc(sizeof (Space));
		sp[i]->mountp = strdup(flist[i]);
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
sort_spacetab(Space ** sp)
{
	int i;

	/* parameter check */
	if (sp == (Space **)NULL)
		return;

	for (i = 0; sp[i]; i++)
		;
	qsort((char *) sp, (size_t) i, sizeof (Space *), do_cmp);
}

/*
 * zero_spacetab()
 *	Clear the sizes in the space tab.
 * Parameters:
 *	sp 	- valid space table pointer
 * Return:
 *	none
 * Status:
 *	publice
 */
void
zero_spacetab(Space ** sp)
{
	int i;

	/* parameter check */
	if (sp == (Space **)NULL)
		return;

	for (i = 0; sp[i]; i++) {
		sp[i]->bused = 0;
		sp[i]->fused = 0;
		sp[i]->bavail = 0;
		sp[i]->touched = 0;
	}
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

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
 *	v1	- first Space table entry
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

	Space **sp1 = (Space **)v1;
	Space **sp2 = (Space **)v2;

	s1 = (*sp1)->mountp;
	s2 = (*sp2)->mountp;

	return ((-1) * strcoll(s1, s2));
}

/*
 * alloc_spacetab()
 *	Create a Space table pointer array of size 'buckets' out of the heap.
 *	This does not allocate the Space structures, but only the array of
 *	structure pointers.
 * Parameters:
 *	buckets	- number of Space structures in the array
 * Return:
 *	NULL	- invalid parameter or alloc failure
 *	# > 0	- pointer to newly allocated Space table structure
 * Status:
 *	private
 * Note:
 *	alloc routine
 */
static Space **
alloc_spacetab(int buckets)
{
	/* parameter check */
	if (buckets <= 0)
		return ((Space **)NULL);
	else
		return ((Space **) xcalloc(buckets * sizeof (Space *)));
}
