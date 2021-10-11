#ifndef lint
#pragma ident   "@(#)sp_calc.c 1.38 95/06/26 SMI"
#endif  lint
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

#include <errno.h>
#include <limits.h>


/* Local Statics and Constants */

static	int 		*upg_overhead;

#define	FS_BLK		8192
#define	FS_FRAG		1024
#define	D_ENTRIES	12	/* Number of blocks before indirection */

/* Externals */

extern	char	*slasha;
extern	int	upg_state;
#ifdef DEBUG
extern	FILE	*ef;
#endif

/* Public Function Prototypes */
u_int	swi_min_req_space(u_int);

/* Library Function Prototypes */
void	begin_space_chk(Space **);
void	begin_qspace_chk(Space **);
void	end_space_chk(void);
void	add_file(char *, daddr_t, daddr_t, int);
void	add_file_blks(char *, daddr_t, daddr_t, int);
void	add_contents_entry(char *, daddr_t, Modinfo *);
void	set_upg_fs_overhead(int *);
int	do_stat_file(char *fname);
void	chk_sp_init(void);
void	add_fs_overhead(Space **, int);
void	add_upg_fs_overhead(Space **);
daddr_t	get_spooled_size(char *);
#ifdef NOTUSED
int	add_dev_usage(char *);
#endif

/* Local Function Prototypes */

static void	do_sp_init(void);
static void	track_dirs(char *, int);
static daddr_t	nblks(daddr_t);
static void	record_file_info(char *, daddr_t, daddr_t, int);
static void	record_save_file(dev_t, daddr_t, daddr_t);
static int	stat_each_path(Node *, caddr_t);
static int 	find_devid(char *, int);
static daddr_t	how_many(daddr_t, daddr_t);
static daddr_t	blkrnd(daddr_t, daddr_t);
static void	new_dir_entry(char *);
static void	del_pathtab(Node *);

static int	sp_init_done = 0;

static int	fs_nindir;
static int	direct;
static daddr_t	sindir;
static daddr_t	se_blks;

static List	*pathlist;
Space		**cur_stab;

/*
 * Path table data structure
 */
typedef struct pathtab {
	char	*name;
	daddr_t	bytes;
	dev_t	st_dev;
} Pathtab;

/*
 * min_req_space ()
 *	Adds UFS overhead to 'size' to determine the smallest size a
 *	file system should be to hold 'size'.
 * Parameters:
 *	size	- number of units used as basis of calculation
 * Return:
 *	# >= 0	- number of units representing the minimum filesystem
 *		  size required to hold 'size' data given overhead.
 * Status:
 *	public
 */
u_int
swi_min_req_space(u_int size)
{
	u_int	s;
	float	fsize;
	
	fsize = size;
	
	/* assume a 10%  + 6.8% fixed overhead rate */
	if (size > 0)
		s = (u_int) (fsize / ((1.0 - 0.1)*(1.0 - 0.068)));
	else
		s = 0;

	return (s);
}

/*
 * begin_space_chk()
 * Begin a space checking session by allocating resources.
 * Parameters:	sp  -
 * Return:	none
 */
void
begin_space_chk(Space **sp) 
{
#ifdef DEBUG
	if (sp == NULL) {
		(void) fprintf(ef, "DEBUG: begin_space_chk():\n");
		(void) fprintf(ef, "Function called with NULL sp pointer.\n");
		return;
	}
#endif
	pathlist = getlist();
	chk_sp_init();
	cur_stab = sp;
}

/*
 * Begin a space checking session by allocating resources. Quick means
 * don't track directory space.
 */
void
begin_qspace_chk(Space **sp)
{
#ifdef DEBUG
	if (sp == NULL) {
		(void) fprintf(ef, "DEBUG: begin_qspace_chk():\n");
		(void) fprintf(ef, "Function called with NULL sp pointer.\n");
		return;
	}
#endif
	pathlist = NULL;
	chk_sp_init();
	cur_stab = sp;
}


/*
 * end_space_chk()
 * End the current space checking session. Run the directory list
 * and free resources.
 * Parameters:	none
 * Return:	none
 */
void
end_space_chk(void) 
{
	if (pathlist != NULL) {
		(void) walklist(pathlist, stat_each_path, (caddr_t)NULL);
		dellist(&pathlist);
	}
	cur_stab = (Space **)NULL;
	pathlist = (List *)NULL;
}

/*
 * Convert a files space usage from bytes to blocks and add it to the
 * current spacetab.
 * Note: track_dirs must be called before record_file_info.
 */
void
add_file(char *fname, daddr_t size, daddr_t inodes, int type)
{
	if (cur_stab == (Space **)NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_file():\n");
		(void) fprintf(ef, "Called while NULL cur_stab ptr.\n");
#endif
		return;
	}

	if (pathlist != NULL)
		track_dirs(fname, type);

	record_file_info(fname, nblks(size), inodes, type);
}

/*
 * Add a files space usage in blocks to the current spacetab.
 */
void
add_file_blks(char *fname, daddr_t size, daddr_t inodes, int type)
{
	if (cur_stab == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_file_blks():\n");
		(void) fprintf(ef, "Called while NULL cur_stab ptr.\n");
#endif
		return;
	}
	if (pathlist != NULL)
		track_dirs(fname, type);
	record_file_info(fname, size, inodes, type);
}

/*
 * Add an entry from a contents file to a spacetab.
 */
void
add_contents_entry(char *fname, daddr_t size, Modinfo *mp)
{
	chk_sp_init();

	if (strncmp("/var", fname, strlen("/var")) == 0)
		mp->m_deflt_fs[VAR_FS] += nblks(size);
	else if (strncmp("/usr/openwin", fname, strlen("/usr/openwin")) == 0)
		mp->m_deflt_fs[USR_OWN_FS] += nblks(size);
	else if (strncmp("/usr", fname, strlen("/usr")) == 0)
		mp->m_deflt_fs[USR_FS] += nblks(size);
	else if (strncmp("/opt", fname, strlen("/opt")) == 0)
		mp->m_deflt_fs[OPT_FS] += nblks(size);
	else if (strncmp("/export/root", fname, strlen("/export/root")) == 0)
		mp->m_deflt_fs[EXP_ROOT_FS] += nblks(size);
	else if (strncmp("/export/home", fname, strlen("/export/home")) == 0)
		mp->m_deflt_fs[EXP_HOME_FS] += nblks(size);
	else if (strncmp("/export/exec", fname, strlen("/export/exec")) == 0)
		mp->m_deflt_fs[EXP_EXEC_FS] += nblks(size);
	else if (strncmp("/export", fname, strlen("/export")) == 0)
		mp->m_deflt_fs[EXPORT_FS] += nblks(size);
	else
		mp->m_deflt_fs[ROOT_FS] += nblks(size);
}


/*
 * For each entry on pathlist add it to the current spacetab.
 */
int
stat_each_path(Node *np, caddr_t cp)
{
	Pathtab	*datap;

	cp = NULL;		/* Make lint shut up */
	datap = np->data;

	record_file_info(datap->name, nblks(datap->bytes), 1, 0);
	return (0);
}

/*
 * Determine the bucket where the space will get credited and do it.
 */
void
record_file_info(char *path, daddr_t blks, daddr_t inodes, int type)
{
	register int	i;

	if (upg_state) {
		register dev_t devid;

		if ((devid = find_devid(path, type)) != 0) {
			for (i = 0; cur_stab[i]; i++) {
				if (cur_stab[i]->fsi == NULL) break;
				if (cur_stab[i]->fsi->st_dev == devid) {
					cur_stab[i]->bused += blks;
					cur_stab[i]->fused += inodes;
					if (upg_state & SP_UPG_EXTRA)
						return;
					cur_stab[i]->touched = 1;
					return;
				}
			}
		}
	}

	/*
	 * Try to match exact name.
	 */
	for (i = 0; cur_stab[i]; i++) {
		if (strcmp(cur_stab[i]->mountp, path) == 0) {
			cur_stab[i]->bused += blks;
			cur_stab[i]->fused += inodes;
			return;
		}
	}

	/*
	 * Must be a full pathname. Determine which mountp bucket.
	 */
	for (i = 0; cur_stab[i]; i++) {
		if (strncmp(cur_stab[i]->mountp, path,
		    strlen(cur_stab[i]->mountp)) == 0) {
			cur_stab[i]->bused += blks;
			cur_stab[i]->fused += inodes;
			break;
		}
	}
}


/*
 * Based on a devid credit some space for upgrades final_space_chk.
 */
static void
record_save_file(dev_t devid, daddr_t blks, daddr_t inodes)
{
	register int	i;

	for (i = 0; cur_stab[i]; i++) {
		if (cur_stab[i]->fsi == NULL) break;
		if (cur_stab[i]->fsi->st_dev == devid) {
			cur_stab[i]->bused += blks;
			cur_stab[i]->fused += inodes;
			return;
		}
	}
}

daddr_t
how_many(daddr_t num, daddr_t blk)
{
	return ((num + (blk - 1)) / blk);
}

daddr_t
blkrnd(daddr_t num, daddr_t blk)
{
	return (how_many(num, blk) * blk);
}

/*
 * do_sp_init()
 *	Set parameters used to calculate the number of blocks a file
 *	will use.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
do_sp_init(void)
{
	fs_nindir = FS_BLK / sizeof (daddr_t);
	direct = D_ENTRIES * FS_BLK;
	sindir = direct + (fs_nindir * FS_BLK);
	se_blks = FS_BLK;

#ifdef FILEDUMP
	ef = fopen("/tmp/space_dump", "w");
	if (ef == (FILE *) NULL)
		ef = stderr;
#endif

	sp_init_done = 1;
}

/*
 * nblks()
 * Given the size of a file return the number of frag size blocks it will use.
 * Parameters:	bytes	- number of bytes in the file
 * Return:	number of fragments
 * Note: We only consider up to double indirection.
 */

daddr_t
nblks(daddr_t bytes)
{
	daddr_t   size, frags;
	daddr_t	  de_blks;

	if (bytes < direct) {
		size = blkrnd(bytes, FS_FRAG);
	/*
	 * After direct no fragments.
	 */
	} else if (bytes < sindir) {
		size = blkrnd(bytes, FS_BLK);
		size += FS_BLK;
	} else {
		/* Data blocks */
		size = blkrnd(bytes, FS_BLK);

		/* Number of second level indirection blocks */
		de_blks = (how_many(((size - sindir)/ fs_nindir), FS_BLK) * FS_BLK);

		/* data blocks + second level indirection blocks +
		 * first level indirection block used by double indirection +
		 * indirection block used for single indirection.
		 */
		size += de_blks + se_blks + FS_BLK;
	}

	frags = size / FS_FRAG;
	return (frags);
}

/*
 * Keep track of space used by directories.
 * The calculation takes the current directory length,
 * adds the path length, adds the 'inode/length' information (8),
 * and then rounds to the nearest word boundry.
 * If this is applied to '.' and '..', you should get a total length of 24,
 * '.'  = (comp_len = 1) + 8 = 9 rnd 4 = 12
 * '..' = (comp_len = 2) + 8 = 10 + 12 = 22 rnd 4 = 24
 */
void
track_dirs(char *pathname, int type)
{
	int	comp_len = 0;
	char	path_comp[MAXPATHLEN], *comp_name, *cp;
	Node	*np;
	Pathtab	*datap;

#ifdef DEBUG
	if (*pathname != '/') {
		(void) fprintf(ef, "DEBUG: track_dirs():\n");
		(void) fprintf(ef, "Warning: Path not absolute: %s\n",
		    pathname);
	}
#endif
	if (type & SP_MOUNTP)
		return;

	(void) strcpy(path_comp, pathname);

	/*
	 * If this is not a directory.
	 */
	if (!(type & SP_DIRECTORY)) {
		/*
		 * Determine directory component.
		 */
		cp = strrchr(path_comp, '/');
		if (cp == (char *) NULL)
			return;
		if (cp == path_comp)
			(void) strcpy(path_comp, "/");
		else {
			comp_len = strlen((cp + 1));
			*cp = '\0';
		}
	}

	/*
	 * If this exists on our list and comp_len is set then credit
	 * the space and return.
	 */
	np = findnode(pathlist, path_comp);
	if (np) {
		/*
		 * If file not a dir then comp_len set above.
		 */
		if (comp_len != 0) {
			datap = np->data;
			datap->bytes = blkrnd((datap->bytes + comp_len + 8), 4);
		}
		return;
	} else {
		new_dir_entry(path_comp);
	}

	if (strcmp(path_comp, "/") == 0)		/* is "/"  */
		return;

	while (path_comp && *path_comp) {
		comp_name = strrchr(path_comp, '/');

		if (comp_name == (char *) NULL)
			break;
		else {
			comp_len = strlen(comp_name+1);

			if (comp_name != path_comp)	/* not "/" */
				*comp_name = '\0';
			else 				/* is "/"  */
				*(comp_name + 1) = '\0';
		}

		np = findnode(pathlist, path_comp);
		if (np) {
			datap = np->data;
			datap->bytes = blkrnd((datap->bytes + comp_len + 8), 4);
			break;
		} else {
			new_dir_entry(path_comp);
		}

		if (strcmp(path_comp, "/") == 0)	/* is "/"  */
			break;
	}
}


/*
 * new_dir_entry()
 * Add a new directory entry in the list. Initialize new structure
 * with a byte size of 24 which reflects entries "." and ".." .
 * Parameters:	pathname	- 
 * Return:	none
 */
void
new_dir_entry(char *pathname)
{
	Node		*np;
	Pathtab		*datap;
	struct stat	sbuf;

	np = getnode();
	datap = (void *) xcalloc(sizeof (Pathtab));
	datap->name = xstrdup(pathname);
	datap->bytes = 24;

	if (stat(pathname, &sbuf) < 0) {
		if (errno != ENOENT) {
			int dummy = 0; dummy += 1; /* Make lint shut up */
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: new_dir_entry():\n");
			(void) fprintf(ef, "stat failed for file %s\n", pathname);
			perror("stat");
#endif
		}
	} else {
		datap->st_dev = sbuf.st_dev;
	}
	np->data = datap;
	np->key = xstrdup(datap->name);
	np->delproc = del_pathtab;
	(void) addnode(pathlist, np);
}

/*
 * del_pathtab()
 * Free the referenced by the path table structure.
 * Parameters:	np	- pointer to node structure referencing the path table
 * Return:	none
 * Note:	dealloc routine
 */
void
del_pathtab(Node *np)
{
	Pathtab	*datap;

	datap = np->data;
	if (datap->name != (char *)NULL)
		(void) free(datap->name);

	free(datap);
}


/*
 * find_devid()
 * Look for the device id of the directory containing this file.
 * Note: This should be called after track_dirs.
 * Parameters:	pathname	-
 *		type		-
 * Return:	0	- Error: invalid parameter, or unknown device ID
 *		# > 0	- device ID associated with 'pathname'
 */
int
find_devid(char *pathname, int type)
{
	char	*cp, path_comp[MAXPATHLEN];
	Pathtab	*datap;
	Node	*np;

	if (pathlist == NULL)
		return (0);

	(void) strcpy(path_comp, pathname);

	if (!(type & SP_DIRECTORY)) {
		/*
		 * Determine directory component.
		 */
		cp = strrchr(path_comp, '/');
		if (cp == (char *) NULL)
			return (0);
		if (cp == path_comp)
			(void) strcpy(path_comp, "/");
		else
			*cp = '\0';
	}

	/*
	 * Reverse search the full path until we find a devid.
	 */
	while (np = findnode(pathlist, path_comp)) {
		datap = (void *) np->data;
		if (datap->st_dev != 0)
			return (datap->st_dev);

		cp = strrchr(path_comp, '/');
		if (cp == (char *) NULL)
			return (0);
		if (cp == path_comp)
			(void) strcpy(path_comp, "/");
		else
			*cp = '\0';
	}

	return (0);
}

/*
 * do_stat_file()
 * Parameters:	fname	- 
 * Return:	SP_ERR_STAT
 *		SUCCESS
 */
int
do_stat_file(char *fname)
{
	struct 	stat sbuf;

	if (fname == NULL)
		return (SUCCESS);

	if (path_is_readable(fname) != SUCCESS) {
		set_sp_err(SP_ERR_STAT, errno, fname);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: dp_stat_file()\n");
		(void) fprintf(ef, "File to be saved not readable: %s\n",
		    fname);
#endif
		return (SP_ERR_STAT);
	}

	if (*fname != '/') {
		set_sp_err(SP_ERR_PATH_INVAL, 0, fname);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: do_stat_file()\n");
		(void) fprintf(ef, "Save file not an absolute path: %s\n",
		    fname);
#endif
		return (SP_ERR_PATH_INVAL);
	}

	if (lstat(fname, &sbuf) < 0) {
		set_sp_err(SP_ERR_STAT, errno, fname);
#ifdef DEBUG
		(void) fprintf(ef, "do_stat_file():\n");
		(void) fprintf(ef, "lstat failed for file %s\n", fname);
		perror("lstat");
#endif
		return (SP_ERR_STAT);
	}
	if (!(sbuf.st_mode & S_IFBLK) && !(sbuf.st_mode & S_IFCHR))
		record_save_file(sbuf.st_dev, nblks(sbuf.st_size), 1);

	return (SUCCESS);
}

/*
 * chk_sp_init()
 *
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
chk_sp_init(void)
{
	slasha = get_rootdir();

	if (*slasha == '\0')
		slasha = NULL;

	if (sp_init_done == 0)
		do_sp_init();
}

/*
 * add_fs_overhead()
 *	Add file system overhead to all entries in the
 *	software space table.
 * Parameters:	sp	- space table used in the calculation
 *		flags	- 
 * Return:	none
 */
void
add_fs_overhead(Space **sp, int flags)
{
	int 	i;
	daddr_t oh;

	/* parameter check */
	if (sp == (Space **)NULL)
		return;

	oh = percent_free_space();
	if (oh == 0)
		return;

	for (i = 0; sp[i]; i++) {
		if (sp[i]->bused == 0)
			continue;

		if (strcmp(sp[i]->mountp, "swap") == 0)
			continue;

		/*
		 * this calculation is a result of empirical tests
		 * which show that file systems have approximately
		 * 1240 sectors (620K) of fixed overhead and a
		 * 3.5% variable overhead rate. The 10% is the
		 * default free space overhead.
		 */
		sp[i]->bused = min_req_space(sp[i]->bused);

		if (flags & SP_CNT_DEVS) {
			/* Add extra 5% to account for slinks in /dev */
			if (strcmp("/", sp[i]->mountp) == 0)
				sp[i]->bused = (sp[i]->bused * 105) / 100;
		}

		/*
		 * The over head is a percentage of total file system
		 * size. So it is going to be some fraction of the overal
		 * softwares size.  It is exactly 1/(1-(percent_free/100))
		 */
		if (sp[i]->fused != 0)
			sp[i]->fused = (sp[i]->fused / (1 - (float) oh/100));
	}
}

/*
 * add_upg_fs_overhead()
 * Calculate the upgrade overhead for each filesystem in the 'sp'
 * space table, and factor each of the 'bused' and 'fused' fields.
 * Parameters:	sp	- space table to use in calculation
 * Return:	none
 */
void
add_upg_fs_overhead(Space **sp)
{
	int 	i;
	daddr_t	oh;

	/* parameter check */
	if (sp == (Space **)NULL)
		return;

	if (upg_overhead == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "add_upg_fs_overhead():");
		(void) fprintf(ef, "called before overhead values set.\n");
#endif
		return;
}
	for (i = 0; sp[i]; i++) {
		if (sp[i]->bused == 0)
			continue;

		oh = upg_percent_free_space(sp[i]->mountp);
		if (oh == 0)
			continue;
		oh += 100;
		sp[i]->bused = (sp[i]->bused * oh) / 100;

		if (sp[i]->fused == 0)
			continue;
		sp[i]->fused = (sp[i]->fused * oh) / 100;
	}
}

/*
 * upg_percent_free_space()
 * Calculate the upgrade overhead # of blocks required for the 'mountp' mount 
 * point.
 * Parameters:	mountp	- mount point to calculate
 * Return:	# >= 0	- # of overhead blocks
 */
daddr_t
upg_percent_free_space(char * mountp)
{
		daddr_t   oh;

		if (strcmp(mountp, "swap") == 0)
			oh = 0;
		else if (strcmp(mountp, "/") == 0)
			oh = upg_overhead[ROOT_FS];
		else if (strcmp(mountp, "/usr") == 0)
			oh = upg_overhead[USR_FS];
		else if (strcmp(mountp, "/usr/openwin") == 0)
			oh = upg_overhead[USR_OWN_FS];
		else if (strcmp(mountp, "/opt") == 0)
			oh = upg_overhead[OPT_FS];
		else if (strcmp(mountp, "/var") == 0)
			oh = upg_overhead[VAR_FS];
		else if (strcmp(mountp, "/export/exec") == 0)
			oh = upg_overhead[EXP_EXEC_FS];
		else if (strcmp(mountp, "/export/root") == 0)
			oh = upg_overhead[EXP_ROOT_FS];
		else if (strcmp(mountp, "/export/swap") == 0)
			oh = upg_overhead[EXP_SWAP_FS];
		else if (strcmp(mountp, "/export/home") == 0)
			oh = upg_overhead[EXP_HOME_FS];
		else if (strcmp(mountp, "/export") == 0)
			oh = upg_overhead[EXPORT_FS];
		else
			oh = 0;

		return (oh);
}

#ifdef NOTUSED
/*
 * Most devices are not specified in pkgmap files. This routine pokes
 * around the installed system to estimate how much space are used
 * by these.
 */
int
add_dev_usage(char *rootdir)
{
	int	blks = 0, inodes = 0;
	char	*slash = "/";
	char	buf[BUFSIZ], command[MAXPATHLEN + 20];
	char	path[MAXPATHLEN], path2[MAXPATHLEN];
	FILE	*pp;

	if (rootdir == NULL)
		rootdir = slash;

	if (*rootdir != '/') {
		set_sp_err(SP_ERR_PATH_INVAL, 0, rootdir);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_dev_usage():\n");
		(void) fprintf(ef, "rootdir %s not absolute path.\n", rootdir);
#endif
		return (SP_ERR_PATH_INVAL);
	}

	if (slasha) {
		if (!do_chroot(slasha))
			return (SP_ERR_CHROOT);
	}

	if (strcmp(rootdir, "/") == 0) {
		(void) strcpy(path, "/dev");
		(void) strcpy(path2, "/devices");
	} else {
		(void) sprintf(path, "%s/dev", rootdir);
		(void) sprintf(path2, "%s/devices", rootdir);
	}

	if (path_is_readable(path) != SUCCESS) {
		set_sp_err(SP_ERR_STAT, errno, path);
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SP_ERR_STAT);
	}

	(void) sprintf(command, "/usr/bin/du -sk %s", path);
	if ((pp = popen(command, "r")) == NULL) {
		set_sp_err(SP_ERR_POPEN, -1, command);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_dev_usage():\n");
		(void) fprintf(ef, "popen failed for du.\n");
#endif
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SP_ERR_POPEN);
	}
	if (fgets(buf, BUFSIZ, pp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		(void) sscanf(buf, "%d %*s", &blks);
	}
	(void) pclose(pp);

	(void) sprintf(command, "/usr/bin/find %s %s -print | /usr/bin/wc -l",
	    path, path2);
	if ((pp = popen(command, "r")) == NULL) {
		set_sp_err(SP_ERR_POPEN, -1, command);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_dev_usage(): popen failed for find.\n");
#endif
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SP_ERR_POPEN);
	}

	if (fgets(buf, BUFSIZ, pp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		(void) sscanf(buf, "%d", &inodes);
	}
	(void) pclose(pp);

	add_file_blks(path, (daddr_t) blks, (daddr_t) inodes, SP_DIRECTORY);

	if (slasha) {
		if (!do_chroot("/"))
			return (SP_ERR_CHROOT);
	}

	return (SUCCESS);
}

#endif /* NOTUSED */

/*
 * get_spooled_size()
 * Get the number of blocks used by the filesystem tree specified by 'pkgdir'.
 * Parameter:	pkgdir	- directory to summarize
 * Return:	# >= 0	- block count
 */
daddr_t
get_spooled_size(char * pkgdir)
{
	daddr_t	blks = 0;
	char	buf[BUFSIZ], command[MAXPATHLEN + 20];
	FILE	*pp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_spooled_size");
#endif

	if (pkgdir == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: get_spooled_size(): pkgdir = NULL.\n");
#endif
		return (SP_ERR_PARAM_INVAL);

	}

	if (path_is_readable(pkgdir) != SUCCESS) {
		set_sp_err(SP_ERR_STAT, errno, pkgdir);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: get_spooled_size(): path unreadable: %s.\n", pkgdir);
#endif
		return (SP_ERR_STAT);
	}

	(void) sprintf(command, "/usr/bin/du -sk %s", pkgdir);
	if ((pp = popen(command, "r")) == NULL) {
		set_sp_err(SP_ERR_POPEN, -1, command);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: get_spooled_size(): popen failed for du.\n");
#endif
		return (SP_ERR_POPEN);
	}
	if (fgets(buf, BUFSIZ, pp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		(void) sscanf(buf, "%ld %*s", &blks);
	}
	(void) pclose(pp);

	return (blks);
}

/*
 * set_upg_fs_overhead()
 * Set the upgrade overhead state variable to the specified value.
 * Parameters:	a	- value to set overhead variable
 * Returns:	none
 */
void
set_upg_fs_overhead(int * a)
{
	upg_overhead = a;
}
