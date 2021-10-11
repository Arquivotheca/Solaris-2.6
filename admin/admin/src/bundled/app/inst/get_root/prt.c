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
#ident "@(#)prt.c 1.2 93/11/23"
#endif	/* !lint */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <stdarg.h>

static char	*ProgName = NULL; 	/* Set via set_prog_name() */

char *
set_prog_name(char *name)
{
	if (name == NULL)
		return (NULL);
	if ((ProgName = strdup(name)) == NULL) {
		(void) fprintf(stderr,
		    "set_prog_name(): strdup(name) failed.\n");
		exit(1);
	}
	ProgName = strrchr(ProgName, '/');
	if (!ProgName++)
		ProgName = name;

	return (ProgName);
}

char *
get_prog_name(void)
{
	return (ProgName);
}

void
perr(char *fmt, ...)
{
	va_list	ap;
	static char	mbuf[BUFSIZ];
	int		save_errno;

	save_errno = errno;
	va_start(ap, fmt);
	(void) vsprintf(mbuf, fmt, ap);
	(void) fprintf(stdout, "echo %s: %s\n", ProgName, strerror(save_errno));
	va_end(ap);
	(void) fflush(stdout);
}
