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
#ifndef lint
#ident "@(#)get_root.c 1.3 93/11/29"
#endif	/* !lint */

#include <ftw.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mkdev.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#include "prt.h"

extern	int	find_name(const char *fname, const struct stat *statb,
				int mode);
static	void	pr_v(const char *var_name, const char *var_value, int csh);

char	*options = "Cb:c:er:t:";

int	block_nam = 0;
char	*block_var = "bdev_name";
int	char_nam = 0;
char	*char_var = "cdev_name";
int	csh = 0;
int	show_type = 0;
char	*type_var = "fstype";
char	*root = "/devices";
int	env = 0;

struct statvfs	statvfsb;

main(int argc, char **argv)
{
	int		ret;
	char		buf[BUFSIZ];
	extern int	optind;
	extern char	*optarg;
	int		optchar;

	(void) set_prog_name(argv[0]);

	while ((optchar = getopt(argc, argv, options)) != EOF) {
		switch (optchar) {
		   case 'C':		/* format for csh execution */
			csh = 1;
			break;

		   case 'b':		/* Look for block device name */
			block_var = optarg;
			block_nam = 1;
			break;

		   case 'c':		/* Look for raw device name */
			char_var = optarg;
			char_nam = 1;
			break;

		   case 'e':		/* Put in environment */
			env = 1;
			break;

		   case 'r':		/* root to look for device. */
			root = optarg;
			break;

		   case 't':		/* print out the fstype */
			type_var = optarg;
			show_type = 1;
			break;

		   default:
			exit(usage());
		}
	}

	if (optind >= argc)
		exit(usage());

	for (/* void */; optind < argc; optind++) {
		if ((ret = statvfs(argv[optind], &statvfsb)) < 0) {
			perr("statvfs()");
			exit(1);
		} else {
			if (show_type)
				pr_v(type_var, statvfsb.f_basetype, csh);
			if (!strcmp(statvfsb.f_basetype, "nfs") ||
			    !strcmp(statvfsb.f_basetype, "rfs")) {
				if (block_nam)
					pr_v(block_var, "-", csh);
				if (char_nam)
					pr_v(char_var, "-", csh);
				continue;
			}
			if (!block_nam && !char_nam)
				continue;
			if (ftw(root, find_name, 20) == -1) {
			    perr("ftw()");
			    exit(1);
			}
		}
	}
	exit(0);
}

int
usage()
{
	(void) fprintf(stdout, "echo \"Usage: %s ", get_prog_name());
	(void) fprintf(stdout,
	    "[-e] [-r <SearchRoot>] [-t <TypeVar>] [-b <BlockVar>] " \
	    "[-c <CharVar] [-C]<file or directory>\"\n");

	return (1);
}

extern int
find_name(const char *fname, const struct stat *statb, int mode)
{
	char		*name, *asctime();
	struct tm	*localtime();

	if (mode == FTW_NS)
		return (0);
	if (S_ISCHR(statb->st_mode)) {
		if (major(statvfsb.f_fsid) == major(statb -> st_rdev) &&
		    minor(statvfsb.f_fsid) == minor(statb -> st_rdev))
			if (char_nam)
				    pr_v(char_var, fname, csh);
	}
	if (S_ISBLK(statb->st_mode)) {
		if (major(statvfsb.f_fsid) == major(statb -> st_rdev) &&
		    minor(statvfsb.f_fsid) == minor(statb -> st_rdev))
		    if (block_nam)
			    pr_v(block_var, fname, csh);
	}
	return (0);
}

static void
pr_v(const char *var_name, const char *var_value, int csh)
{
	if (csh) {
		if (env)
			(void) printf("setenv %s='%s';\n", var_name,
			    var_value);
		else
			(void) printf("set %s='%s';\n", var_name, var_value);
	} else {
		if (env) {
			(void) printf("%s='%s';\n", var_name, var_value);
			(void) printf("export %s;\n", var_name);
		} else
			(void) printf("%s='%s';\n", var_name, var_value);
	}
}
