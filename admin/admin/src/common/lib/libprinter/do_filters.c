/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)do_filters.c	1.7	94/10/13 SMI"

/*
 * do_filters
 *
 * This file has routines for adding/deleting/listing print filters
 */

#include <string.h>
#include "printer_impl.h"


static
char *PS_filters[] = {
	"download",
	"dpost",
	"postdaisy",
	"postdmd",
	"postio",
	"postior",
	"postmd",
	"postplot",
	"postprint",
	"postreverse",
	"posttek",
	""
};

static int
add_filter(char *filtername)
{
	char lpfiltercmd[PRT_MAXSTRLEN];
	int err;

	/*
	 * Check to see if the filter is already there:
	 * (For Now, we are just going to add it anyway)
	 */

	(void) sprintf(lpfiltercmd, "%s -f %s -F %s/%s.fd", PRT_LPFILTER,
			filtername, PRT_FILTERDIR, filtername);

	err = do_system(lpfiltercmd);

	return (err);
}

static int
del_filter(char *filtername)
{
	char lpfiltercmd[PRT_MAXSTRLEN];
	int err;

	/*
	 * Remove a filter via the "-x" switch to lpfilter
	 */

	(void) sprintf(lpfiltercmd, "PRT_LPFILTER -f %s -x", filtername);

	err = do_system(lpfiltercmd);

	return (err);
}

/*
 * Restore (?) a filter
 */
static int
restore_filter(char *filtername)
{
	char lpfiltercmd[PRT_MAXSTRLEN];
	int err;

	/*
	 * Remove a filter via the "-x" switch to lpfilter
	 */

	(void) sprintf(lpfiltercmd, "PRT_LPFILTER -f %s -x", filtername);

	err = do_system(lpfiltercmd);

	return (err);
}

/*ARGSUSED*/
static int
view_filter(char *filtername)
{
	/*
	 * View a filter via the "-l" switch to lpfilter
	 */

	/*
	 * haven't implemented anything here yet
	 */

	return (PRINTER_SUCCESS);
}

static int
list_filters(void)
{
	char lpfiltercmd[PRT_MAXSTRLEN];
	int err;

	/*
	 * List all filters via the "-l" switch to lpfilter
	 */

	(void) sprintf(lpfiltercmd, "PRT_LPFILTER -f all -l");

	err = do_system(lpfiltercmd);

	return (err);
}



int
setup_postscript_filters(void)
{
	char **filptr;
	int err;

	filptr = &PS_filters[0];
	while (strcmp(*filptr, "") != 0) {
		err = add_filter(*filptr++);
		if (err != PRINTER_SUCCESS) {
/*
 *			return (err);
 */
			return (PRINTER_SUCCESS);
		}
	}
	return (PRINTER_SUCCESS);
}

