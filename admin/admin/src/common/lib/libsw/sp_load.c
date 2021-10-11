#ifndef lint
#pragma ident   "@(#)sp_load.c 1.34 96/02/08 SMI"
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

#include <ctype.h>

/* Public Function Prototypes */

int	sp_read_pkg_map(char *, char *, char *, char *, int);
int	sp_read_space_file(char *, char *, char *);
int	sp_load_contents(Product *prod1, Product *prod2);
void	set_sp_err(int, int, char *);

/* Local Function Prototypes */

static void	log_sp_err(int, int, char *);

extern	char	*slasha;
extern	int	upg_state;
extern	int	in_final_upgrade_stage;
extern	int	doing_add_service;

int	sp_warn = 0;

#ifdef DEBUG
extern	FILE	*ef;
#endif

extern	Modinfo *find_owning_inst();
extern	int	match_missing_file();
extern	int	errno;

int sp_err_code, sp_err_subcode;
char *sp_err_path = NULL;

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
		int flags)
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
	 * stat the pkgmap file and get it's size
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

		add_file(fullpath, fsize, inodes, type);
	}

	mclose(mp);

	(void) sprintf(buf, "var/sadm/pkg/%s/pkgmap", pkgdir);
	set_path(fullpath, rootdir_p, NULL, buf);

	(void) sprintf(buf, "var/sadm/pkg/%s/save", pkgdir);
	set_path(fullpath, rootdir_p, NULL, buf);
	add_file(fullpath, 0, 1, SP_DIRECTORY);

	set_path(fullpath, rootdir_p, NULL, "var/sadm/install");
	add_file(fullpath, 0, 1, SP_DIRECTORY);

	if (doing_add_service == 1) {
		/*
		 * use the size of the pkgmap file as an approximation of
		 * the size added to the contents file.
		 * Pkgadd/pkgrm make a tmp copy of the contents file so we
		 * need 2*sizeof(contents_file).
		 */
		set_path(fullpath, rootdir_p, NULL, "var/sadm/install/contents");
		add_file(fullpath, (pkgmapsize * 2), 1, 0);
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
	int	n, is_instance, ftype, is_partial;
	int	err = 0;
	daddr_t	size, inodes;
	char	*rootdir;
	char	contname[MAXPATHLEN];
	char	path[MAXPATHLEN], fullpath[MAXPATHLEN];
	char	static_line[BUFSIZ];
	char	*line;
	char	package[256], *pkg_p, base_pkg[256];
	char	*cp, type;
	FILE	*fp;
	Modinfo	*orig_mod, *cur_mod;
	Node	*np;
	int	malloced_space = 0;

	if (prod1 == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: sp_load_contents():\n");
		(void) fprintf(ef, "prod1 is a NULL pointer\n");
#endif
		return (SP_ERR_PARAM_INVAL);
	}

	chk_sp_init();
	sp_warn = 0;

	if (slasha) {
		if (!do_chroot(slasha))
			return (SP_ERR_CHROOT);
	}

	set_path(contname, prod1->p_rootdir, NULL, "var/sadm/install/contents");
	fp = fopen(contname, "r");
	if (fp == (FILE *) NULL) {
		set_sp_err(SP_ERR_OPEN, errno, contname);
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: sp_load_contents():\n");
		(void) fprintf(ef, "Open failed: file %s .\n", contname);
#endif
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SP_ERR_OPEN);
	}

	while (fgets(static_line, BUFSIZ, fp)) {

		if (static_line[strlen(static_line) - 1] != '\n') {
			int	num_bufs = 1;

			line = xmalloc(BUFSIZ);
			strcpy(line, static_line);
			while (line[strlen(line) - 1] != '\n') {
				num_bufs++;
				line = (char *) xrealloc(line,
				    BUFSIZ * num_bufs);
				(void) fgets(static_line, BUFSIZ, fp);
				strcat(line, static_line);
			}
			malloced_space = 1;
		} else
			line = (char *)&static_line;

		line[strlen(line) - 1] = '\0';
		package[0] = '\0';
		is_instance = 0;
		is_partial = 0;
		ftype = 0;

		if (line[0] == '#' || line[0] == '\0' || line[0] == ':')
			continue;

		(void) sscanf(line, "%s %c", path, &type);

		switch (type) {
		/*
		 * Regular, editable and volatile Files
		 */
		case 'f':
		case 'v':
		case 'e':
			inodes = 1;

			(void) sscanf(line,
			    "%s %*s %*s %*s %*s %*s %*s %*s %*s %s",
				path, package);

			if (!isalpha((unsigned) package[0])) {
				if (package[0] == '\0') {
					err = SP_ERR_CORRUPT_CONTENTS;
					sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
					log_sp_err(SP_ERR_CORRUPT_CONTENTS,
					    SP_WARN_UNEXPECTED_LINE_FORMAT,
					    line);
					continue;
				}
				is_partial = 1;
			} else {
				n = sscanf(line,
				    "%s %*c %*s %*s %*s %*s %ld %*s %*s %s",
				    path, &size, package);
				if (n < 3) {
					err = SP_ERR_CORRUPT_CONTENTS;
					sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
					log_sp_err(SP_ERR_CORRUPT_CONTENTS,
					    SP_WARN_UNEXPECTED_LINE_FORMAT,
					    line);
					continue;
				}
			}
			break;
		/*
		 * Symbolic links
		 */
		case 's':
			inodes = 1;

			n = sscanf(line, "%s %*s %*s %s",
				path, package);
			if (n < 2) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			if (!isalpha((unsigned) package[0]))
				is_partial = 1;

			cp = strchr(path, '=');
			if (cp == NULL) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			size = strlen(cp + 1);
			*cp = '\0';
			break;
		/*
		 * Hard links
		 */
		case 'l':
			inodes = 1;
			size = 0;

			n = sscanf(line, "%s %*s %*s %s",
				path, package);
			if (n < 2) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			cp = strchr(path, '=');
			if (cp == NULL) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			*cp = '\0';
			break;
		/*
		 * Char/Block Special Files
		 */
		case 'c':
		case 'b':
			inodes = 1;
			size = 0;

			n = sscanf(line, "%s %*s %*s %*s %*s %*s %*s %*s %s",
				path, package);
			if (n < 2) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			break;
		/*
		 * Pipe Special Files
		 */
		case 'p':
			size = 0;
			inodes = 0;

			n = sscanf(line, "%s %*s %*s %*s %*s %*s %s",
				path, package);
			if (n < 2) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			break;
		/*
		 * Directories
		 */
		case 'd':
		case 'x':
			ftype |= SP_DIRECTORY;
			size = 0;
			inodes = 0;

			n = sscanf(line, "%s %*s %*s %*s %*s %*s %s",
				path, package);
			if (n < 2) {
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_UNEXPECTED_LINE_FORMAT;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_UNEXPECTED_LINE_FORMAT, line);
				continue;
			}
			if (path[strlen(path) - 1] == '/') {
				path[strlen(path) - 1] = '\0';
			}
			break;
		default:
			continue;
		}

		/*
		 * If file doesn't exist then continue.
		 * Only matches in final_space_chk().
		 */
		if (match_missing_file(path))
			continue;

		/*
		 * Attempt to stat the file. If it fails just continue;
		 */
		if (is_partial) {
			struct stat sbuf;

			sp_warn |= SP_WARN_PARTIAL_INSTALL_FOUND;
			set_path(fullpath, prod1->p_rootdir, NULL, path);
			if (lstat(fullpath, &sbuf) < 0)
				continue;
			else
				size = sbuf.st_size;
		}

		/*
		 * We're adding up installed space for upgrade final space chk.
		 */
		if (upg_state & SP_UPG_INSTALLED_CHK) {
			set_path(fullpath, prod1->p_rootdir, NULL, path);
			add_file(fullpath, size, inodes, ftype);
			continue;
		}

		/*
		 * We are going to assign this space to a Modinfo struct.
		 *
		 * Extract package name from the possible form:
		 * [status]<package_name>.<instance>:<class>
		 */
		cp = strchr(package, ':');
		if (cp != NULL)
			*cp = '\0';
		if (!isalpha((unsigned) package[0]))
			pkg_p = package + 1;
		else
			pkg_p = package;

		(void) strcpy(base_pkg, pkg_p);
		cp = strchr(base_pkg, '.');
		if (cp) {
			if (strpbrk(cp, "0123456789")) {
				is_instance = 1;
				*cp = '\0';
			}
		}
		/*
		 * Only count first package this file is listed for.
		 * Handle case where a package is part of a servers
		 * environment and also a service.
		 * If pkg is an instance find it's owner.
		 */
		if ((prod2) && (strncmp(path, "/export/exec", 12) == 0)) {
			rootdir = prod2->p_rootdir;
			np = findnode(prod2->p_packages, base_pkg);
			if (np == NULL) {
#ifdef DEBUG
				(void) fprintf(ef, "DEBUG: sp_load_contents():\n");
				(void) fprintf(ef, "findnode failed to find %s\n",
				    base_pkg);
#endif
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |=  SP_WARN_FINDNODE_FAILED;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_FINDNODE_FAILED, pkg_p);
				continue;
			}
			orig_mod = (Modinfo *) np->data;
			if (is_instance)
				cur_mod = find_owning_inst(pkg_p, orig_mod);
			else
				cur_mod = orig_mod;
		} else {
			rootdir = prod1->p_rootdir;
			np = findnode(prod1->p_packages, base_pkg);
			if (np == NULL) {
#ifdef DEBUG
				(void) fprintf(ef, "DEBUG: sp_load_contents():\n");
				(void) fprintf(ef, "findnode failed to find %s\n",
				    base_pkg);
#endif
				err = SP_ERR_CORRUPT_CONTENTS;
				sp_warn |= SP_WARN_FINDNODE_FAILED;
				log_sp_err(SP_ERR_CORRUPT_CONTENTS,
				    SP_WARN_FINDNODE_FAILED, pkg_p);
				continue;
			}
			orig_mod = (Modinfo *) np->data;
			if (is_instance)
				cur_mod = find_owning_inst(pkg_p, orig_mod);
			else
				cur_mod = orig_mod;
		}

		if (cur_mod == NULL) {
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: sp_load_contents():\n");
			(void) fprintf(ef, "cur_mod NULL for pkg: %s\n",
			    base_pkg);
#endif
			sp_warn |= SP_WARN_FIND_OWNING_INST_FAILED;
			log_sp_err(SP_ERR_CORRUPT_CONTENTS,
			    SP_WARN_FIND_OWNING_INST_FAILED, pkg_p);
			continue;
		}
		set_path(fullpath, rootdir, NULL, path);

		if (upg_state & SP_UPG_SPACE_CHK) {
			if (meets_reqs(cur_mod))
				add_file(fullpath, size, inodes, ftype);
		} else if (size != 0)
			add_contents_entry(fullpath, size, cur_mod);

		if (malloced_space) {
			malloced_space = 0;
			free(line);
		}
	}
	(void) fclose(fp);

	if (slasha) {
		if (!do_chroot("/"))
			return (SP_ERR_CHROOT);
	}

	if (err)
		return (err);
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
sp_read_space_file(char * s_path, char * rootdir_p, char * basedir_p)
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
				strtoul(f2, (char **)NULL, 10), SP_DIRECTORY);
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
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

static void
log_sp_err(int errcode, int specific_err, char *arg)
{
	static int err_count = 0;

	if (in_final_upgrade_stage) {
		err_count++;
		if (err_count > 8)
			return;
		switch (errcode) {
		case SP_ERR_CORRUPT_CONTENTS:
			switch (specific_err) {
			case SP_WARN_UNEXPECTED_LINE_FORMAT:
				printf(dgettext("SUNW_INSTALL_SWLIB",
"Illegal /var/sadm/install/contents entry:\n%s\n"), arg);
				break;

			case SP_WARN_FINDNODE_FAILED:
			case SP_WARN_FIND_OWNING_INST_FAILED:
				printf(dgettext("SUNW_INSTALL_SWLIB",
"Package instance \"%s\" appears in /var/sadm/install/contents,\nbut is not installed.\n"), arg);
				break;
			}
			break;
		}
	}
}

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
