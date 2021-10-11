#ifndef lint
#pragma ident "@(#)driver.c 1.1 96/05/10 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	driver.c
 * Group:	libspmistore
 * Description:	Test module to drive unit tests
 */

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/fs/ufs_fs.h>
#include "spmistore_lib.h"
#include "spmicommon_api.h"

/* --------------------- Test Interface ------------------------ */

main(int argc, char **argv, char **env)
{
	Disk_t *	dp;
	int		n;
	char *		file = NULL;
	int		printboot = 0;

	while ((n = getopt(argc, argv, "bx:d:h")) != -1) {
		switch (n) {
		case 'b':
			printboot++;
			break;
		case 'd':
			(void) SetSimulation(SIM_SYSDISK, 1);
			file = xstrdup(optarg);
			(void) printf("Using %s as an input file\n", file);
			break;
		case 'x':
			(void) set_trace_level(atoi(optarg));
			break;
		case 'h':
			(void) printf(
			    "Usage: %s [-x <debug level>] [-d <disk file>]\n",
			    basename(argv[0]));
			exit(1);
		}
	}

	(void) set_rootdir("/a");
	n = DiskobjInitList(file);
	if (n < 0) {
		(void) printf("Error %d returned from disk load\n", n);
		exit (1);
	}

	(void) printf("%d disks found\n\n", n);
	(void) printf("-----------------------------------\n");

	WALK_DISK_LIST(dp) {
		print_disk(dp, NULL);
		(void) printf("-----------------------------------\n");
	}

	if (printboot)
		BootobjPrint();

	exit (0);
}
