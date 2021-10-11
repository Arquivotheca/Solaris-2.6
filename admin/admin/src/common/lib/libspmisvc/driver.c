#ifndef lint
#pragma ident "@(#)driver.c 1.5 96/05/09 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	driver.c
 * Group:	none
 * Description:
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fs/ufs_fs.h>
#include "spmisvc_lib.h"
#include "spmistore_api.h"
#include "spmisoft_lib.h"
#include "spmicommon_api.h"

main(int argc, char **argv, char **env)
{
	Disk_t *	list = NULL;
	Disk_t *	dp;
	StringList *	slices = NULL;
	StringList *	releases = NULL;
	StringList *	sp;
	int		n;
	char *		file = NULL;
	char *		rootmount = "/a";
	int		u = 0;

	while ((n = getopt(argc, argv, "x:ud:L")) != -1) {
		switch (n) {
		case 'd':
			(void) SetSimulation(SIM_SYSDISK, 1);
			file = strdup(optarg);
			(void) printf("Using %s as an input file\n", file);
			break;
		case 'x':
			(void) set_trace_level(atoi(optarg));
			break;
		case 'L':
			rootmount = "/";
		case 'u':
			u++;
			break;
		default:
			(void) fprintf(stderr,
		"Usage: %s [-x <level>] [-u] [-L] [-d <disk file>]\n",
				argv[0]);
			exit (1);
		}
	}

	(void) set_rootdir(rootmount);
	/* initialize the disk list only for non-direct runs */
	if (!streq(rootmount, "/")) {
		n = DiskobjInitList(file);
		(void) printf("Disks found - %d\n", n);
	}

	if (u > 0) {
		SliceFindUpgradeable(&slices, &releases);
		if (slices != NULL) {
			(void) printf("Upgradeable slices:\n");
			WALK_LIST(sp, slices)
				(void) printf("\t%s\n", sp->string_ptr);

			(void) printf("Upgradeable releases:\n");
			WALK_LIST(sp, releases)
				(void) printf("\t%s\n", sp->string_ptr);

			(void) StringListFree(slices);
			(void) StringListFree(releases);
		} else
			(void) printf("No upgradeable slices.\n");
	}

	exit (0);
}
