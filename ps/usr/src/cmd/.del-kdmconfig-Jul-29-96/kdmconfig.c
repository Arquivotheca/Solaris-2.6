/*
 * kdmconfig.c: Main routine for kd driver app-config program.
 *
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 *
 *
 */

#ident "@(#)kdmconfig.c 1.14 95/06/15 SMI"

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include "kdmconfig.h"
#include "except.h"
#include "kdmconfig_msgs.h"

static int silent_output = 1; /* defaults to silent */

int	force_prompt = 0;	/* defeat silent probe */

void
verb_msg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (!silent_output) {
		(void) vprintf(fmt, ap);
		(void) fflush(stdout);
	}

	va_end(ap);
}

static void
do_usage(void)
{
	(void) fprintf(stderr, "%s\n", KDMCONFIG_MSGS(KDMCONFIG_USAGE));
	exit(1);
}

static void
_check_superuser()
{
	if (getuid()) {
		ui_error(KDMCONFIG_MSGS(KDMCONFIG_SUPERUSER), 1);
		exit(-1);
	}
}


main(int argc, char **argv)
{
	int goodarg = 0;
	int c;
	char arg;

	/*
	 * Hard code the locale to C for the moment; this is a
	 * workaround for bugid 1171714; kdmconfig hanging with
	 * multibyte character sets.
	 */
	(void) setlocale(LC_ALL, "C");
	(void) textdomain(KDMCONFIG_MSGS_TEXTDOMAIN);

	_check_superuser();

	while ((c = getopt(argc, argv, "fvcus:")) != EOF)
		switch (c) {
		case 'f':
			force_prompt++;
			break;
		case 'v':
			silent_output=0;
			break;
		case 'c':
			goodarg++;
			arg = c;
			break;
		case 'u':
			goodarg++;
			arg = c;
			break;
		case 's':
			set_server_mode(optarg);
			break;
		default:
			do_usage();
			break;
	}

	if (goodarg != 1)
		do_usage();

	switch (arg) {
	case 'c':
		do_config();
		break;
	case 'u':
		do_unconfig();
		break;
	default:
		break;
	}

	return (0);
}
