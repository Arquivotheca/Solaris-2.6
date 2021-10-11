/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)install.c	1.4	96/04/18 SMI"	/* SVr4.0 1.2	*/

 
/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

 Copyright (c) 1986,1987,1988,1989,1996, by Sun Microsystems, Inc.
 All rights reserved.

	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)installcmd.c 1.8 88/08/07 SMI"; /* from UCB 1.4 7/25/87 */
#endif not lint

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>

#define	DEF_GROUP	"staff"		/* default group */
#define	DEF_OWNER	"root"		/* default owner */
#define	DEF_MODE	0755		/* default mode */

char *group = DEF_GROUP;
char *owner = DEF_OWNER;
int mode    = DEF_MODE;
int sflag = 0;
struct passwd *pp;
struct group *gp;
extern int errno;
int copy();
void usage();

main(argc, argv)
	int	argc;
	char	**argv;
{
	extern char	*optarg;
	extern int	optind;
	struct stat	stb;
	char	*dirname;
	int	ch;
	int	i;
	int	rc;
	int	dflag = 0;
	int	gflag = 0;
	int	oflag = 0;
	int	mflag = 0;

	(void) setlocale(LC_ALL, "");

#if !defined(TEXT_DOMAIN)
#define TEXT_DOMAIN "SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);

	while ((ch = getopt(argc, argv, "dcg:o:m:s")) != EOF)
		switch((char)ch) {
		case 'c':
			break;	/* always do "copy" */
		case 'd':
			dflag++;
			break;
		case 'g':
			gflag++;
			group = optarg;
			break;
		case 'm':
			mflag++;
			mode = atoo(optarg);
			break;
		case 'o':
			oflag++;
			owner = optarg;
			break;
		case 's':
			sflag++;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* get group and owner id's */
	if (!(gp = getgrnam(group))) {
		fprintf(stderr, gettext("install: unknown group %s.\n"), group);
		exit(1);
	}
	if (!(pp = getpwnam(owner))) {
		fprintf(stderr, gettext("install: unknown user %s.\n"), owner);
		exit(1);
	}

	if (dflag) {		/* install a directory */
		int exists = 0;

		if (argc != 1)
			usage();
		dirname = argv[0];
		if (mkdirp(dirname, 0777) < 0) {
			exists = errno == EEXIST;
			if (!exists) {
				fprintf(stderr, gettext("install: mkdir: %s: %s\n"), dirname, strerror(errno));
				exit(1);
			}
		}
		if (stat(dirname, &stb) < 0) {
			fprintf(stderr, gettext("install: stat: %s: %s\n"), dirname, strerror(errno));
			exit(1);
		}
		if ((stb.st_mode&S_IFMT) != S_IFDIR) {
			fprintf(stderr, gettext("install: %s is not a directory\n"), dirname);	
		}
		/* make sure directory setgid bit is inherited */
		mode = (mode & ~S_ISGID) | (stb.st_mode & S_ISGID);
		if (mflag && chmod(dirname, mode)) {
			fprintf(stderr, gettext("install: chmod: %s: %s\n"), dirname, strerror(errno));
			if (!exists)
				(void) unlink(dirname);
			exit(1) ;
		}
		if (oflag && chown(dirname, pp->pw_uid, -1) && errno != EPERM) {
			fprintf(stderr, gettext("install: chown: %s: %s\n"), dirname, strerror(errno));
			if (!exists)
				(void) unlink(dirname);
			exit(1) ;
		}
		if (gflag && chown(dirname, -1, gp->gr_gid) && errno != EPERM) {
			fprintf(stderr, gettext("install: chgrp: %s: %s\n"), dirname, strerror(errno));
			if (!exists)
				(void) unlink(dirname);
			exit(1) ;
		}
		exit(0);
	}

	if (argc < 2)
		usage();

        if (argc > 2) {		/* last arg must be a directory */
                if (stat(argv[argc-1], &stb) < 0)
                        usage();
                if ((stb.st_mode&S_IFMT) != S_IFDIR) 
                        usage();
        }
        rc = 0;
        for (i = 0; i < argc-1; i++)
                rc |= install(argv[i], argv[argc-1]);
        exit(rc);
	/* NOTREACHED */
}

int
install(from, to)
	char *from, *to;
{
	int to_fd;
	int devnull;
	int status = 0;
	char *path;
	struct stat from_sb, to_sb;
	static char pbuf[MAXPATHLEN];
	char buf[MAXPATHLEN + 10];

	/* check source */
	if (stat(from, &from_sb)) {
		fprintf(stderr, gettext("install: %s: %s\n"), from, strerror(errno));
		return (1);
	}
	/* special case for removing files */
	devnull = !strcmp(from, "/dev/null");
	if (!devnull && !((from_sb.st_mode&S_IFMT) == S_IFREG)) {
		fprintf(stderr, gettext("install: %s isn't a regular file.\n"), from);
		return (1);
	}

	/* build target path, find out if target is same as source */
	if (!stat(path = to, &to_sb)) {
		if ((to_sb.st_mode&S_IFMT) == S_IFDIR) {
			char *C, *strrchr();

			(void) sprintf(path = pbuf, "%s/%s", to, (C = strrchr(from, '/')) ? ++C : from);
			if (stat(path, &to_sb))
				goto nocompare;
		}
		if ((to_sb.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stderr, gettext("install: %s isn't a regular file.\n"), path);
			return (1);
		}
		if (to_sb.st_dev == from_sb.st_dev && to_sb.st_ino == from_sb.st_ino) {
			fprintf(stderr, gettext("install: %s and %s are the same file.\n"), from, path);
			return (1);
		}
		/* unlink now... avoid ETXTBSY errors later */
		(void) unlink(path);
	}

nocompare:
	/* open target, set mode, owner, group */
	if ((to_fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0)) < 0) {
		fprintf(stderr, gettext("install: %s: %s\n"), path, strerror(errno));
		return (1);
	}
	if (fchmod(to_fd, mode)) {
		fprintf(stderr, gettext("install: chmod: %s: %s\n"), path, strerror(errno));
		status = 1;
		close(to_fd);
		goto inst_done;
	}
	if (!devnull) {
		status = copy(from, to_fd, path);  /* copy */
		close(to_fd);
	}
	if (sflag) {
		sprintf(buf, "strip %s", path);
		system(buf);
	}
	if (chown(path, pp->pw_uid, gp->gr_gid) && errno != EPERM) {
		fprintf(stderr, gettext("install: chown: %s: %s\n"), path, strerror(errno));
		status = 1;
	}

inst_done:
	if (status)
		(void) unlink(path);
	return (status);
}

/*
 * copy --
 *	copy from one file to another
 */
int
copy(from_name, to_fd, to_name)
	int to_fd;
	char *from_name, *to_name;
{
	int n, from_fd;
	int status = 0;
	char buf[MAXBSIZE];

	if ((from_fd = open(from_name, O_RDONLY, 0)) < 0) {
		fprintf(stderr, gettext("install: open: %s: %s\n"), from_name, strerror(errno));
		return (1);
	}
	while ((n = read(from_fd, buf, sizeof(buf))) > 0)
		if (write(to_fd, buf, n) != n) {
			fprintf(stderr, gettext("install: write: %s: %s\n"), to_name, strerror(errno));
		status = 1;
		goto copy_done;
		}
	if (n == -1) {
		fprintf(stderr, gettext("install: read: %s: %s\n"), from_name, strerror(errno));
		status = 1;
		goto copy_done;
	}

copy_done:
	(void) close(from_fd);
	return (status);
}

/*
 * atoo --
 *      octal string to int
 */
int
atoo(str)
        register char   *str;
{        
        register int    val;
 
        for (val = 0; isdigit(*str); ++str)
                val = val * 8 + *str - '0';
        return(val);
}


/*
 * usage --
 *	print a usage message and die
 */
void
usage()
{
	fputs(gettext("usage: install [-cs] [-g group] [-m mode] [-o owner] file ...  destination\n"), stderr);
	fputs(gettext("       install  -d   [-g group] [-m mode] [-o owner] dir\n"), stderr);
	exit(1);
}

/*
 * mkdirp --
 *	make a directory and parents if needed
 */
int
mkdirp(dir, mode)
	char *dir;
	int mode;
{
	int err;
	char *slash;
	char *strrchr();
	extern int errno;

	if (mkdir(dir, mode) == 0)
		return (0);
	if (errno != ENOENT)
		return (-1);
	slash = strrchr(dir, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = mkdirp(dir, 0777);
	*slash = '/';
	if (err)
		return (err);
	return mkdir(dir, mode);
}
