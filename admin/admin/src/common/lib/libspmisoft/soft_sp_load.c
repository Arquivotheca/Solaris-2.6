#ifndef lint
#pragma ident "@(#)soft_sp_load.c 1.7 96/06/10 SMI"
#endif
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include "spmisoft_lib.h"
#include "sw_space.h"
#include "find_mod.h"

#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>
#include <stdlib.h>

/* Public Function Prototypes */

int	sp_read_pkg_map(char *, char *, char *, char *, int, FSspace **);
int	sp_read_space_file(char *, char *, char *, FSspace **);
int	sp_load_contents(Product *prod1, Product *prod2);
void	set_sp_err(int, int, char *);

/* Library Function Prototypes */
void	set_add_service_mode(int);

/* Local Function Prototypes */
static int	match_missing_file(char *);

extern	char	*slasha;
extern	int	upg_state;

int	sp_warn = 0;
int	doing_add_service = 0;

#ifdef DEBUG
extern	FILE	*ef;
#endif

extern	Modinfo *find_owning_inst();
extern	int	errno;

int sp_err_code, sp_err_subcode;
char *sp_err_path = NULL;

/* global variable */
struct missing_file *missing_file_list = NULL;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * sp_read_pkg_map()
 *	Read a pkg map file. flags = 0 or SP_CNT_DEVS. Most devs are not
 *	listed. For the final upgrade space check ignore dev entries now
 *	and fudge later using "du".
 * Parameters:
 *	pkgmap_path -
 *	pkgdir	    -
 *	rootdir_p   -
 *	basedir_p   -
 *	flags	    -
 * Return:
 * Status:
 *	public
 */
int
sp_read_pkg_map(char *pkgmap_path,
		char *pkgdir,
		char *rootdir_p,
		char *basedir_p,
		int flags,
		FSspace **sp)
{
	char		f0[100], f1[100], f2[100], f3[100];
	char		f4[100], f5[100], f6[100], f7[100];
	char		path[MAXPATHLEN + 1], fullpath[MAXPATHLEN + 1];
	char		buf[BUFSIZ + 1], *path_p, *cp;
	int		type, inodes;
	daddr_t 	pkgmapsize, fsize;
	MFILE		*mp;
	struct stat	sbuf;

	if ((path_is_readable(pkgmap_path) == FAILURE) ||
	    ((mp = mopen(pkgmap_path)) == (MFILE *) NULL)) {
		set_sp_err(SP_ERR_OPEN, errno, pkgmap_path);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: sp_read_pkg_map():\n");
		(void) fprintf(ef, "Can't open %s\n", pkgmap_path);
#endif
		return (SP_ERR_OPEN);
	}

	/*
	 * stat the pkgmap file and get its size
	 */
	if (lstat(pkgmap_path, &sbuf) < 0) {
		set_sp_err(SP_ERR_STAT, errno, pkgmap_path);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: sp_read_pkg_map():\n");
		(void) fprintf(ef, "lstat failed for file %s\n", pkgmap_path);
		perror("lstat");
#endif
		return (SP_ERR_STAT);
	}
	pkgmapsize = (daddr_t) sbuf.st_size;


	if (slasha) {
		if (!do_chroot(slasha))
			return (SP_ERR_CHROOT);
	}

	while (mgets(buf, BUFSIZ, mp)) {
		ProgressAdvance(PROG_PKGMAP_SIZE, strlen(buf),
		   VAL_NEWPKG_SPACE , pkgdir);
		buf[strlen(buf) - 1] = '\0';
		type = 0;

		if (buf[0] == '#' || buf[0] == '\0' || buf[0] == ':')
			continue;

		(void) sscanf(buf, "%s %s", f0, f1);

		switch (*f1) {
		/*
		 * Regular, editable and volatile Files
		 */
		case 'f':
		case 'v':
		case 'e':
			(void) sscanf(buf, "%s %s %s %s %s %s %s %s",
			    f0, f1, f2, f3, f4, f5, f6, f7);
			path_p = f3;
			fsize = strtoul(f7, (char **)NULL, 10);
			inodes = 1;
			break;
		/*
		 * Char/Block/Pipe Special Files
		 */
		case 'c':
		case 'b':
		case 'p':
			(void) sscanf(buf, "%s %s %s %s", f0, f1, f2, f3);
			path_p = f3;
			fsize = 0;
			inodes = 1;
			break;
		/*
		 * Hard Links
		 */
		case 'l':
			(void) sscanf(buf, "%s %s %s %s", f0, f1, f2, f3);
			if (cp = strchr(f3, '='))
				*cp = '\0';
			path_p = f3;
			fsize = 0;
			inodes = 0;
			break;
		/*
		 * Symbolic links
		 */
		case 's':
			(void) sscanf(buf, "%s %s %s %s", f0, f1, f2, f3);
			if (cp = strchr(f3, '='))
				*cp = '\0';
			path_p = f3;
			fsize = strlen(cp+1);
			inodes = 1;
			break;
		/*
		 * Directories
		 */
		case 'd':
		case 'x':
			(void) sscanf(buf, "%s %s %s %s", f0, f1, f2, f3);
			(void) strcpy(path, f3);
			if (path[strlen(path) - 1] == '/') {
				path[strlen(path) - 1] = '\0';
			}
			type |= SP_DIRECTORY;
			path_p = path;
			/*
			 * size and inodes captured in stat_each_path()
			 */
			fsize = 0;
			inodes = 0;
			break;
		/*
		 * Packaging files
		 */
		case 'i':
			(void) sscanf(buf, "%s %s %s %s", f0, f1, f2, f3);
			if (strncmp(f2, "pkginfo", 7) == 0) {
				(void) sprintf(path, "%s/%s/%s",
				    "/var/sadm/pkg", pkgdir, f2);
				path_p = path;
				fsize = strtoul(f3, (char **)NULL, 10);
			} else {
				(void) sprintf(path, "%s/%s/%s/%s",
				    "/var/sadm/pkg", pkgdir, "install", f2);
				path_p = path;
				fsize = strtoul(f3, (char **)NULL, 10);
			}
			inodes = 1;
			break;
		default:
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: sp_read_pkg_map():\n");
			(void) fprintf(ef, "Unrecognized pkgmap line. %s: %s\n",
			    pkgdir, buf);
#endif
			continue;
		}

#ifdef DEBUG
		if (check_path_for_vars(path_p)) {
			(void) fprintf(ef, "DEBUG: sp_read_pkg_map():\n");
			(void) fprintf(ef, "Warning: File has vars: %s Pkg: %s:\n",
				path_p, pkgdir);
		}
#endif

		if (!(flags & SP_CNT_DEVS)) {
			if (strncmp(path_p, "dev/", 4) == 0) continue;
			if (strncmp(path_p, "devices/", 8) == 0) continue;
			if (strncmp(path_p, "/dev/", 5) == 0) continue;
			if (strncmp(path_p, "/devices/", 9) == 0) continue;
		}

		if (*path_p != '/') {
			set_path(fullpath, rootdir_p, basedir_p, path_p);
		} else {
			set_path(fullpath, rootdir_p, NULL, path_p);
		}

		add_file(fullpath, fsize, inodes, type, (FSspace **)sp);
	}

	mclose(mp);

	(void) sprintf(buf, "var/sadm/pkg/%s/pkgmap", pkgdir);
	set_path(fullpath, rootdir_p, NULL, buf);

	(void) sprintf(buf, "var/sadm/pkg/%s/save", pkgdir);
	set_path(fullpath, rootdir_p, NULL, buf);
	add_file(fullpath, 0, 1, SP_DIRECTORY, (FSspace **)sp);

	set_path(fullpath, rootdir_p, NULL, "var/sadm/install");
	add_file(fullpath, 0, 1, SP_DIRECTORY, (FSspace **)sp);

	if (doing_add_service == 1) {
		/*
		 * use the size of the pkgmap file as an approximation of
		 * the size added to the contents file.
		 * Pkgadd/pkgrm make a tmp copy of the contents file so we
		 * need 2*sizeof(contents_file).
		 */
		set_path(fullpath, rootdir_p, NULL, "var/sadm/install/contents");
		add_file(fullpath, (pkgmapsize * 2), 1, 0, (FSspace **)sp);
	}

	if (slasha) {
		if (!do_chroot("/"))
			return (SP_ERR_CHROOT);
	}

	return (SUCCESS);
}

/*
 * sp_load_contents()
 *
 * Parameters:
 *	prod1	-
 *	prod2	-
 * Return:
 *
 * Status:
 *	public
 */
int
sp_load_contents(Product *prod1, Product *prod2)
{
	int	n;
	off_t	size;
	char	contname[MAXPATHLEN];
	char	fullpath[MAXPATHLEN];
	FILE	*fp;
	static	struct	cfent	centry;
	struct	pinfo	*pp;
	Modinfo	*mi, *cur_mi;
	struct  crsave {
		struct crsave	*next;
		List	*pathlist;
		Modinfo	*crmi;
		ContentsRecord	*cr;
	};
	struct crsave *crsavehead = NULL;
	struct crsave *crp, *crnext;
	char	cur_cr_pkg[64] = "";
	FSspace	**fsp;
	struct	stat	sbuf;
	static	int	first = 1;
	int	type;

	sp_warn = 0;
	if (first) {
		centry.pinfo = NULL;
		first = 0;
	}

	if (slasha) {
		if (!do_chroot(slasha))
			return (SP_ERR_CHROOT);
	}

	set_path(contname, prod1->p_rootdir, NULL, "var/sadm/install/contents");
	fp = fopen(contname, "r");
	if (fp == (FILE *) NULL) {
		set_sp_err(SP_ERR_OPEN, errno, contname);
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SP_ERR_OPEN);
	}

	fsp = get_current_fs_layout();
	while ((n = get_next_contents_entry(fp, &centry)) != 0) { 
		ProgressAdvance(PROG_CONTENTS_LINES, 1, VAL_CONTENTS_SPACE,
		    NULL);

		if (n < 0) {
			/* garbled entry, just skip it */
			continue;
		}
		if (match_missing_file(centry.path))
			continue;

		/*
		 * We are going to assign this space to a Modinfo struct.
		 */
		for (pp = centry.pinfo; pp != NULL; pp = pp->next) {
			mi = map_pinfo_to_modinfo(prod1, pp->pkg);
			if (mi == NULL && prod2 != NULL)
				mi = map_pinfo_to_modinfo(prod2, pp->pkg);
			/*
			 *  If the package is one that could not possibly be
			 *  updated by the upgrade or add_service operation,
			 *  don't add it up.  Let it be considered "extra" space
			 *  on the system.  Note that this isn't sufficient if
			 *  we ever want to support removal of unbundled
			 *  packages through this library (and we want to know
			 *  how much space is freed by removing a particular
			 *  package, so that it can be considered available for
			 *  adding additional packages).
			 */

			if (mi != NULL) {
				if (mi->m_flags & IS_UNBUNDLED_PKG)
					mi = NULL;
			}
			if (mi != NULL)
				break;
		}

		if (mi == NULL)
			continue;	/* no bundled owning pkg; skip it */

		/* prod1 and prod2 (if non-NULL) always have same rootdir */
		set_path(fullpath, prod1->p_rootdir, NULL, centry.path);
		if (lstat(fullpath, &sbuf) < 0)
			continue;

		if (!streq(pp->pkg, cur_cr_pkg)) {
			if (cur_cr_pkg[0] != '\0') {
				/* must save the current package's record */
				for (crp = crsavehead; crp != NULL;
				    crp = crp->next) {
					/*LINTED [var set before used]*/
					if (crp->crmi == cur_mi)
						break;
				}
				if (crp == NULL) {
					crp = (struct crsave *)xcalloc((size_t)
					    sizeof (struct crsave));
					crp->crmi = cur_mi;
					link_to((Item **)&crsavehead,
					    (Item *)crp);
				}
				crp->cr = contents_record_from_stab(fsp,
				    crp->cr);
				crp->pathlist = (List *)(fsp[0]->fsp_internal);
			}

			/* now load the new package's contents record */
			strcpy(cur_cr_pkg, pp->pkg);
			cur_mi = mi;
			for (crp = crsavehead; crp != NULL; crp = crp->next)
				if (crp->crmi == mi)
					break;
			if (crp == NULL) {
				reset_stab(fsp);
				begin_specific_space_sum(fsp);
			} else {
				stab_from_contents_record(fsp, crp->cr);
				fsp[0]->fsp_internal = (void *)(crp->pathlist);
			}
		}
		if (S_ISDIR(sbuf.st_mode))
			type = SP_DIRECTORY;
		else
			type = 0;

		if (S_ISBLK(sbuf.st_mode) || S_ISCHR(sbuf.st_mode))
			size = 0;
		else
			size = sbuf.st_size;

		add_file(fullpath, size, 1, type, (FSspace**)fsp);
	}
	(void) fclose(fp);

	if (cur_cr_pkg[0] != '\0') {
		/* must save the current package's record */
		for (crp = crsavehead; crp != NULL; crp = crp->next)
			if (crp->crmi == cur_mi)
				break;
		if (crp == NULL) {
			crp = (struct crsave *)xcalloc((size_t)
			    sizeof (struct crsave));
			crp->crmi = cur_mi;
			link_to((Item **)&crsavehead, (Item *)crp);
		}
		crp->cr = contents_record_from_stab(fsp, crp->cr);
		crp->pathlist = (List *)(fsp[0]->fsp_internal);
	}

	for (crp = crsavehead; crp != NULL; crp = crnext) {
		stab_from_contents_record(fsp, crp->cr);
		fsp[0]->fsp_internal = (void *)(crp->pathlist);
		end_specific_space_sum(fsp);
		crp->crmi->m_fs_usage = contents_record_from_stab(fsp, crp->cr);
		/* record the space in the running total */
		add_spacetab(fsp, (FSspace **)NULL);
		crnext = crp->next;
		free(crp);
	}

	if (slasha) {
		if (!do_chroot("/"))
			return (SP_ERR_CHROOT);
	}

	return (SUCCESS);
}

/*
 * sp_read_space_file()
 *
 * Parameters:
 *	s_path	  - file pathname to space file
 *	rootdir_p -
 *	basedir_p -
 * Return:
 *	SUCCESS
 *	SP_ERR_CHROOT	- coudn't chroot() to "/a"
 *	SP_ERR_OPEN	- couldn't open specified space file, or couldn't
 *			  mmap (open) it
 * Note:
 *	The space file format is:
 *		<file>	<size in 512 byte blocks> <# inodes>
 */
int
sp_read_space_file(char * s_path, char * rootdir_p, char * basedir_p,
    FSspace **sp)
{
	MFILE		*mp;
	char		fullpath[MAXPATHLEN + 1];
	char		buf[BUFSIZ + 1];
	char		f0[100], f1[100], f2[100];

	if ((path_is_readable(s_path) == FAILURE) ||
	    ((mp = mopen(s_path)) == (MFILE *) NULL)) {
		set_sp_err(SP_ERR_OPEN, errno, s_path);
		return (SP_ERR_OPEN);
	}

	if (slasha) {
		if (!do_chroot(slasha))
			return (SP_ERR_CHROOT);
	}
	/*
	 * read in a line, strip off the '\n', ignore comments and null
	 * lines, and read in the 3 fields
	 */
	while (mgets(buf, BUFSIZ, mp)) {
		buf[strlen(buf) - 1] = '\0';

		if (buf[0] == '#' || buf[0] == '\0' || buf[0] == ':')
			continue;

		(void) sscanf(buf, "%s %s %s", f0, f1, f2);
		set_path(fullpath, rootdir_p, basedir_p, f0);
		/*
		 * Space field is specified in 512 byte blocks, so expand
		 * to bytes
		 */
		add_file(fullpath, (strtoul(f1, (char **)NULL, 10) * 512),
		    strtoul(f2, (char **)NULL, 10), SP_DIRECTORY,
		    (FSspace **)sp);
	}

	(void) mclose(mp);
	/* return "/" to its original state */
	if (slasha) {
		if (!do_chroot("/"))
			return (SP_ERR_CHROOT);
	}
	return (SUCCESS);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

void
set_add_service_mode(int mode)
{
	doing_add_service = mode;
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

void
set_sp_err(int errcode, int specific_err, char *arg)
{
	if (sp_err_path) {
		free(sp_err_path);
		sp_err_path = NULL;
	}
	if (arg)
		sp_err_path = xstrdup(arg);
	sp_err_code = errcode;
	sp_err_subcode = specific_err;
}

static int
match_missing_file(char *path)
{
	int n;
	struct missing_file *missp;

	if (missing_file_list == NULL)
		return (0);
	n = strlen(path);
	path[n] = '/';
	for (missp = missing_file_list; missp != NULL; missp = missp->next) {
		if (strncmp(missp->missing_file_name, path, missp->misslen)
		    == 0) {
			path[n] = '\0';
			return (1);
		}
	}
	path[n] = '\0';
	return (0);
}
