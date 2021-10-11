/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)soladdapp.c	1.1	95/01/16 SMI"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "launcher_api.h"


const char	*arg_usage = "[ -r registry ]"
			     " -n name"
			     " [ -i icon ]"
			     " -e executable"
			     " [ -- args ]";


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
		msg = "error: name already exists in the registry";
		break;
	case 4:
	default:
		msg = "error: registration failed";
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
	int		len;
	const char	*registry = NULL;
	const char	*name = NULL;
	const char	*icon = NULL;
	const char	*executable = NULL;
	char		args[512];
	boolean_t	quiet = B_FALSE;
	boolean_t	usage_err = B_FALSE;
	const char	*ptr;
	const char	*basename;
	SolsticeApp	app_info;


	while ((c = getopt(argc, argv, "qr:n:i:e:")) != EOF) {
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
		case 'i':
			icon = strdup(optarg);
			break;
		case 'e':
			executable = strdup(optarg);
			break;
		case '?':
			usage_err = B_TRUE;
		}
	}

	if (usage_err == B_TRUE || name == NULL || executable == NULL) {

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

	args[0] = 0;
	for ( ; optind < argc; optind++) {
		strcat(args, argv[optind]);
		strcat(args, " ");
	}

	/* remove last space from args */

	if ((len = strlen(args)) > 0) {
		args[len - 1] = 0;
	}

	app_info.name = name;
	app_info.icon_path = icon;
	app_info.app_path = executable;
	app_info.app_args = (const char *)args;

	status = solstice_add_app(&app_info, registry);

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
	case LAUNCH_DUP:
		exit_code = 3;
		break;
	case LAUNCH_ERROR:
	default:
		exit_code = 4;
		break;
	}

	if (exit_code != 0 && quiet == B_FALSE) {
		fprintf(stderr, "%s\n", get_err_msg(exit_code));
	}

	exit(exit_code);
}
