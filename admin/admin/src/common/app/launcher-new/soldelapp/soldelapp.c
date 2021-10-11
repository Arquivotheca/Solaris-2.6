/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)soldelapp.c	1.1	95/01/16 SMI"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "launcher_api.h"


const char	*arg_usage = "[ -r registry ] -n name";


static
const char *
get_err_msg(int exit_code)
{

	const char	*msg;


	switch(exit_code) {
	case 1:
		msg = "error: bad input";
		break;
	case 2:
		msg = "error: registry is locked";
		break;
	case 3:
		msg = "error: can't find name in the registry";
		break;
	case 4:
		msg = "error: can't find registry";
		break;
	case 5:
	default:
		msg = "error: delete from registry failed";
		break;
	}

	return (msg);
}


int
main(int argc, char *argv[])
{

	int		c;
	int		status;
	int		exit_code;
	const char	*registry = NULL;
	const char	*name = NULL;
	boolean_t	quiet = B_FALSE;
	boolean_t	usage_err = B_FALSE;
	const char	*ptr;
	const char	*basename;


	while ((c = getopt(argc, argv, "qr:n:")) != EOF) {
		switch (c)  {
		case 'q':
			quiet = B_TRUE;
			break;
		case 'r':
			registry = strdup(optarg);
			break;
		case 'n':
			name = strdup(optarg);
			break;
		case '?':
			usage_err = B_TRUE;
		}
	}

	if (usage_err == B_TRUE || name == NULL) {

		if ((ptr = strrchr(argv[0], '/')) != NULL) {
			basename = ptr + 1;
		} else {
			basename = argv[0];
		}

		if (quiet == B_FALSE) {
			(void) fprintf(stderr, "usage: %s %s\n",
			    basename, arg_usage);
		}
		exit(1);
	}

	status = solstice_del_app(name, registry);

	switch (status) {
	case LAUNCH_OK:
		exit_code = 0;
		break;
	case LAUNCH_BAD_INPUT:
		exit_code = 1;
		break;
	case LAUNCH_LOCKED:
		exit_code = 2;
		break;
	case LAUNCH_NO_ENTRY:
		exit_code = 3;
		break;
	case LAUNCH_NO_REGISTRY:
		exit_code = 4;
		break;
	case LAUNCH_ERROR:
	default:
		exit_code = 5;
		break;
	}

	if (exit_code != 0 && quiet == B_FALSE) {
		fprintf(stderr, "%s\n", get_err_msg(exit_code));
	}

	exit(exit_code);
}
