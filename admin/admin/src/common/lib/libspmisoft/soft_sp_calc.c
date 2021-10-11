#ifndef lint
#pragma ident "@(#)soft_sp_calc.c 1.6 96/09/26 SMI"
#endif  lint

/* Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved. */

#include "spmisoft_lib.h"
#include "sw_space.h"

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

/* Local Statics and Constants */

#define	FS_BLK		8192
#define	FS_FRAG		1024
#define	D_ENTRIES	12	/* Number of blocks before indirection */
#define	FS_NINDIR	(FS_BLK / sizeof (daddr_t))
#define	FS_DIRECT	(D_ENTRIES * FS_BLK)
#define	SINDIR		(FS_DIRECT + (FS_NINDIR * FS_BLK))
#define	SE_BLKS		FS_BLK

/* Externals */

extern	char	*slasha;
extern	int	upg_state;
#ifdef DEBUG
extern	FILE	*ef;
#endif


/* Library Function Prototypes */
void	begin_global_space_sum(FSspace **);
void	begin_global_qspace_sum(FSspace **);
FSspace	**end_global_space_sum(void);
void	begin_specific_space_sum(FSspace **);
void	begin_specific_qspace_sum(FSspace **);
void	end_specific_space_sum(FSspace **);
void	add_file(char *, daddr_t, daddr_t, int, FSspace **);
void	add_file_blks(char *, daddr_t, daddr_t, int, FSspace **);
int	record_save_file(char *fname, FSspace **);
void	do_spacecheck_init(void);
void	add_contents_record(ContentsRecord *, FSspace **);
void	add_spacetab(FSspace **, FSspace **);

/* Local Function Prototypes */

static void	track_dirs(FSspace **, char *, int);
static daddr_t	nblks(daddr_t);
static void	record_file_info(FSspace **, char *, daddr_t, daddr_t, int);
static int	stat_each_path(Node *, caddr_t);
static int 	find_devid(FSspace **, char *, int);
static daddr_t	how_many(daddr_t, daddr_t);
static daddr_t	blkrnd(daddr_t, daddr_t);
static void	new_dir_entry(char *, List *);
static void	del_pathtab(Node *);
static int	is_an_intermediate_dir_of(char *, char *);
static void	add_contents_brkdn(ContentsBrkdn *, FSspace *);

static FSspace	**cur_stab;

/*
 * Path table data structure
 */
typedef struct pathtab {
	char	*name;
	daddr_t	p_bytes;
	dev_t	p_st_dev;
} Pathtab;

/*
 * begin_global_space_sum()
 * Begin a space checking session by allocating resources.
 * Use the global "cur_stab" pointer to record the space table.
 * Parameters:	sp  -
 * Return:	none
 */
void
begin_global_space_sum(FSspace **sp)
{
	cur_stab = sp;
	cur_stab[0]->fsp_internal = (void *)getlist();
}

/*
 * Begin a space checking session by allocating resources. Quick means
 * don't track directory space.
 */
void
begin_global_qspace_sum(FSspace **sp)
{
	cur_stab = sp;
	cur_stab[0]->fsp_internal = NULL;
}


/*
 * end_global_space_sum()
 * End the current space checking session. Run the directory list
 * and free resources.
 * Parameters:	none
 * Return:	none
 */
FSspace **
end_global_space_sum(void)
{
	FSspace **tstab;

	tstab = cur_stab;

	if (cur_stab) {
		if ((List *)(cur_stab[0]->fsp_internal) != NULL) {
			(void) walklist((List *)(cur_stab[0]->fsp_internal),
			    stat_each_path, (caddr_t)cur_stab);
			dellist((List **)(&(cur_stab[0]->fsp_internal)));
			cur_stab[0]->fsp_internal = NULL;
		}
	}
	cur_stab = (FSspace **)NULL;
	return (tstab);
}

/*
 * begin_specific_space_sum()
 * Begin a space checking session by allocating resources.
 * Use the specified file-space table.  All updates to this table
 *    should specify this file-space table pointer.
 * Parameters:	sp  -
 * Return:	none
 */
void
begin_specific_space_sum(FSspace **sp)
{
	sp[0]->fsp_internal = (void *)getlist();
}

/*
 * Begin a space checking session by allocating resources. Quick means
 * don't track directory space.
 */
void
begin_specific_qspace_sum(FSspace **sp)
{
	sp[0]->fsp_internal = NULL;
}


/*
 * end_specific_space_sum()
 * End the current space checking session. Run the directory list
 * and free resources.
 * Parameters:	none
 * Return:	none
 */
void
end_specific_space_sum(FSspace **sp)
{
	if ((List *)(sp[0]->fsp_internal) != NULL) {
		(void) walklist((List *)(sp[0]->fsp_internal), stat_each_path,
		    (caddr_t)sp);
		dellist((List **)(&(sp[0]->fsp_internal)));
		sp[0]->fsp_internal = NULL;
	}
}

/*
 * Convert a files space usage from bytes to blocks and add it to the
 * current spacetab.
 * Note: track_dirs must be called before record_file_info.
 */
void
add_file(char *fname, daddr_t size, daddr_t inodes, int type_flags,
    FSspace **sp)
{
	if (sp == NULL && cur_stab == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_file():\n");
		(void) fprintf(ef, "Called while NULL cur_stab ptr.\n");
#endif
		return;
	}

	if (sp == NULL)
		sp = cur_stab;
	if ((List *)(sp[0]->fsp_internal) != NULL)
		track_dirs(sp, fname, type_flags);

	record_file_info(sp, fname, nblks(size), inodes, type_flags);
}

/*
 * Add a files space usage in blocks to the current spacetab.
 */
void
add_file_blks(char *fname, daddr_t size, daddr_t inodes, int type_flags,
    FSspace **sp)
{
	if (sp == NULL && cur_stab == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_file_blks():\n");
		(void) fprintf(ef, "Called while NULL cur_stab ptr.\n");
#endif
		return;
	}
	if (sp == NULL)
		sp = cur_stab;
	if ((List *)(sp[0]->fsp_internal) != NULL)
		track_dirs(sp, fname, type_flags);
	record_file_info(sp, fname, size, inodes, type_flags);
}

/*
 * For each entry on the path list add it to the current spacetab.
 */
static int
stat_each_path(Node *np, caddr_t data)
{
	Pathtab	*datap;

	datap = (Pathtab *)(np->data);

	/*LINTED [alignment ok]*/
	record_file_info((FSspace **)data, datap->name, nblks(datap->p_bytes),
	    1, 0);
	return (0);
}

/*
 * Determine the bucket where the space will get credited and do it.
 */
static void
record_file_info(FSspace **sp, char *path, daddr_t blks, daddr_t inodes,
    int type_flags)
{
	register int	i;

	if (sp == NULL)
		sp = cur_stab;
	if (upg_state & SP_UPG) {
		register dev_t devid;

		if ((devid = find_devid(sp, path, type_flags)) != 0) {
			for (i = 0; sp[i]; i++) {
				if (sp[i]->fsp_fsi == NULL) break;
				if (sp[i]->fsp_fsi->f_st_dev == devid) {
					fsp_add_to_field(sp[i],
					    FSP_CONTENTS_PKGD, blks);
					sp[i]->
					    fsp_cts.contents_inodes_used +=
					    inodes;
					sp[i]->fsp_flags |=
					    FS_HAS_PACKAGED_DATA;
					return;
				}
			}
		}
	}

	/*
	 * Try to match exact name.
	 */
	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;
		if (strcmp(sp[i]->fsp_mntpnt, path) == 0) {
			fsp_add_to_field(sp[i], FSP_CONTENTS_PKGD, blks);
			sp[i]->fsp_cts.contents_inodes_used += inodes;
			sp[i]->fsp_flags |= FS_HAS_PACKAGED_DATA;
			return;
		}
	}

	/*
	 * Must be a full pathname. Determine which fsp_mntpnt bucket.
	 */
	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;
		if (strncmp(sp[i]->fsp_mntpnt, path,
		    strlen(sp[i]->fsp_mntpnt)) == 0) {
			fsp_add_to_field(sp[i], FSP_CONTENTS_PKGD, blks);
			sp[i]->fsp_flags |= FS_HAS_PACKAGED_DATA;
			sp[i]->fsp_cts.contents_inodes_used += inodes;
			break;
		}
	}
}

static daddr_t
how_many(daddr_t num, daddr_t blk)
{
	return ((num + (blk - 1)) / blk);
}

static daddr_t
blkrnd(daddr_t num, daddr_t blk)
{
	return (how_many(num, blk) * blk);
}

/*
 * nblks()
 * Given the size of a file return the number of frag size blocks it will use.
 * Parameters:	bytes	- number of bytes in the file
 * Return:	number of fragments
 * Note: We only consider up to double indirection.
 */

static daddr_t
nblks(daddr_t bytes)
{
	daddr_t   size, frags;
	daddr_t	  de_blks;

	if (bytes < FS_DIRECT) {
		size = blkrnd(bytes, FS_FRAG);
	/*
	 * After direct no fragments.
	 */
	} else if (bytes < SINDIR) {
		size = blkrnd(bytes, FS_BLK);
		size += FS_BLK;
	} else {
		/* Data blocks */
		size = blkrnd(bytes, FS_BLK);

		/* Number of second level indirection blocks */
		de_blks =
		    (how_many(((size - SINDIR)/ FS_NINDIR), FS_BLK) * FS_BLK);

		/*
		 * data blocks + second level indirection blocks +
		 * first level indirection block used by double indirection +
		 * indirection block used for single indirection.
		 */
		size += de_blks + SE_BLKS + FS_BLK;
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
track_dirs(FSspace **sp, char *pathname, int type_flags)
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
	if (type_flags & SP_MOUNTP)
		return;

	if (sp == NULL)
		sp = cur_stab;

	(void) strcpy(path_comp, pathname);

	/*
	 * If this is not a directory.
	 */
	if (!(type_flags & SP_DIRECTORY)) {
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
	np = findnode((List *)(sp[0]->fsp_internal), path_comp);
	if (np) {
		/*
		 * If file not a dir then comp_len set above.
		 */
		if (comp_len != 0) {
			datap = np->data;
			datap->p_bytes =
			    blkrnd((datap->p_bytes + comp_len + 8), 4);
		}
		return;
	} else {
		new_dir_entry(path_comp, (List *)(sp[0]->fsp_internal));
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

		np = findnode((List *)(sp[0]->fsp_internal), path_comp);
		if (np) {
			datap = np->data;
			datap->p_bytes =
			    blkrnd((datap->p_bytes + comp_len + 8), 4);
			break;
		} else {
			new_dir_entry(path_comp, (List *)(sp[0]->fsp_internal));
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
static void
new_dir_entry(char *pathname, List *list)
{
	Node		*np;
	Pathtab		*datap;
	struct stat	sbuf;

	np = getnode();
	datap = (void *) xcalloc(sizeof (Pathtab));
	datap->name = xstrdup(pathname);
	datap->p_bytes = 24;

	if (stat(pathname, &sbuf) < 0) {
		if (errno != ENOENT) {
			int dummy = 0; dummy += 1; /* Make lint shut up */
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: new_dir_entry():\n");
			(void) fprintf(ef, "stat failed for file %s\n",
			    pathname);
			perror("stat");
#endif
		}
	} else {
		datap->p_st_dev = sbuf.st_dev;
	}
	np->data = datap;
	np->key = xstrdup(datap->name);
	np->delproc = del_pathtab;
	(void) addnode(list, np);
}

/*
 * del_pathtab()
 * Free the referenced by the path table structure.
 * Parameters:	np	- pointer to node structure referencing the path table
 * Return:	none
 * Note:	dealloc routine
 */
static void
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
 *		type_flags		-
 * Return:	0	- Error: invalid parameter, or unknown device ID
 *		# > 0	- device ID associated with 'pathname'
 */
static int
find_devid(FSspace **sp, char *pathname, int type_flags)
{
	char	*cp, path_comp[MAXPATHLEN];
	Pathtab	*datap;
	Node	*np;

	if ((List *)(sp[0]->fsp_internal) == NULL)
		return (0);

	(void) strcpy(path_comp, pathname);

	if (!(type_flags & SP_DIRECTORY)) {
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
	while (np = findnode((List *)(sp[0]->fsp_internal), path_comp)) {
		datap = (void *) np->data;
		if (datap->p_st_dev != 0)
			return (datap->p_st_dev);

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
 * record_save_file()
 * Parameters:	fname	-
 * Return:	SP_ERR_STAT
 *		SUCCESS
 */
int
record_save_file(char *fname, FSspace **sp)
{
	struct 	stat sbuf;
	int	i;

	if (fname == NULL)
		return (SUCCESS);

	if (sp == NULL)
		sp = cur_stab;

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
	if (!(sbuf.st_mode & S_IFBLK) && !(sbuf.st_mode & S_IFCHR)) {
		for (i = 0; sp[i]; i++) {
			if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
				continue;
			if (sp[i]->fsp_fsi == NULL)
				break;
			if (sp[i]->fsp_fsi->f_st_dev == sbuf.st_dev) {
				fsp_add_to_field(sp[i],
				    FSP_CONTENTS_SAVEDFILES,
				    nblks(sbuf.st_size));
				sp[i]->fsp_cts.contents_inodes_used += 1;
				break;
			}
		}
	}

	return (SUCCESS);
}

/*
 * do_spacecheck_init()
 *
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
do_spacecheck_init(void)
{
	slasha = get_rootdir();

	if (*slasha == '\0')
		slasha = NULL;
}

/*
 * add_contents_record()
 *
 * Add the contents recorded in a ContentsRecord to the specified space
 * table.  The file systems in the space table will always either
 * be identical to or a subset of the file systems in the ContentsRecord.
 * Contents numbers in the ContentsRecord will be collapsed as necessary
 * into the appropriate file systems in the space table.
 *
 * Parameters:
 *	cr	-  the ContentsRecord table to be added to the
 *		current space table.
 *	sp	-  space table to which the ContentsRecord should
 *		be added (if NULL, use the current global space table).
 */
void
add_contents_record(ContentsRecord *cr, FSspace **sp)
{
	int	j, msp_idx;
	FSspace	**msp;
	char	*cr_mntpnt;

	if (sp == NULL)
		sp = cur_stab;
	msp = get_master_spacetab();

	for (; cr != NULL; cr = cr->next) {
		msp_idx = cr->ctsrec_idx;
		cr_mntpnt = msp[msp_idx]->fsp_mntpnt;

		for (j = 0; sp[j] != NULL; j++) {
			if (sp[j]->fsp_flags & FS_IGNORE_ENTRY)
				continue;
			if (is_an_intermediate_dir_of(sp[j]->fsp_mntpnt,
			    cr_mntpnt)) {
				add_contents_brkdn(&(cr->ctsrec_brkdn),
				    sp[j]);
				break;
			}
		}
	}
}

/*
 * add_space_tab()
 *
 * Add the contents recorded in one space table to another space table.
 * The file systems in the space table to be modified will always either
 * be identical to or a subset of the file systems in the space
 * table to be added.
 *
 * Parameters:
 *	fstab	-  the file system space table to be added to the
 *		space table indicated by "sp".
 *	sp	-  space table to which the fstab contents should
 *		be added (if NULL, use the current global space table).
 */
void
add_spacetab(FSspace **fstab, FSspace **sp)
{
	int	i, j;

	if (sp == NULL)
		sp = cur_stab;

	for (i = 0; fstab[i] != NULL; i++) {
		if (fstab[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;

		for (j = 0; sp[j] != NULL; j++) {
			if (sp[j]->fsp_flags & FS_IGNORE_ENTRY)
				continue;
			if (is_an_intermediate_dir_of(sp[j]->fsp_mntpnt,
			    fstab[i]->fsp_mntpnt)) {
				add_contents_brkdn(&(fstab[i]->fsp_cts), sp[j]);
				fsp_add_to_field(sp[j], FSP_CONTENTS_SU_ONLY,
				    fstab[i]->fsp_su_only);
				fsp_add_to_field(sp[j], FSP_CONTENTS_REQD_FREE,
				    fstab[i]->fsp_reqd_free);
				fsp_add_to_field(sp[j], FSP_CONTENTS_UFS_OVHD,
				    fstab[i]->fsp_ufs_ovhd);
				fsp_add_to_field(sp[j], FSP_CONTENTS_ERR_EXTRA,
				    fstab[i]->fsp_err_extra);
				break;
			}
		}
	}
}

/*
 * is_an_intermediate_dir_of()
 *
 * Return TRUE if s1 is an intermediate directory of (or identical to) s2.
 */
static int
is_an_intermediate_dir_of(char *s1, char *s2)
{
	int	len;

	len = strlen(s1);

	/* root is an intermediate directory of everything */
	if (streq(s1, "/"))
		return (1);

	if (strncmp(s1, s2, len) == 0 && (s2[len] == '/' || s2[len] == '\0'))
		return (1);
	else
		return (0);
}

static void
add_contents_brkdn(ContentsBrkdn *cr, FSspace *fs)
{
	fsp_add_to_field(fs, FSP_CONTENTS_PKGD, cr->contents_packaged);
	fsp_add_to_field(fs, FSP_CONTENTS_NONPKG, cr->contents_nonpkg);
	fsp_add_to_field(fs, FSP_CONTENTS_DEVFS, cr->contents_devfs);
	fsp_add_to_field(fs, FSP_CONTENTS_SAVEDFILES, cr->contents_savedfiles);
	fsp_add_to_field(fs, FSP_CONTENTS_PKG_OVHD, cr->contents_pkg_ovhd);
	fsp_add_to_field(fs, FSP_CONTENTS_PATCH_OVHD, cr->contents_patch_ovhd);
	fs->fsp_cts.contents_inodes_used += cr->contents_inodes_used;
}
