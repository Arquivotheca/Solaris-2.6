/*
 * except.c: Exception handling routines.
 *
 * Copyright (c) 1983-1993 Sun Microsystems, Inc.
 *
 */

#ident "@(#)except.c 1.6 93/12/16 SMI"

#include "except.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "ui.h"

static ExceptionMode mode = SILENT_MODE;

void
set_except_mode(ExceptionMode emode)
{
	mode = emode;
}

void
ui_notice(char *text)
{
FILE *fp;
	if (mode == SILENT_MODE && (fp = fopen("/tmp/install_log", "a+"))) {
		(void) fprintf(fp, "%s\n %s\n",
				KDMCONFIG_MSGS(KDMCONFIG_WARNING), text);
		(void) fclose(fp);
	} else {
		ui_error(text, 0);
	}
}

void
ui_error_exit(char *text)
{
	if (mode == SILENT_MODE) {
		(void) fprintf(stderr, "%s %s\n",
				KDMCONFIG_MSGS(KDMCONFIG_ERROR_EXIT), text);
	} else {
		ui_error(text, 1);
	}
	exit(1);
	/* NOTREACHED */
}
