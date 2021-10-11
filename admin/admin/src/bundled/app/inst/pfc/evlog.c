#ifndef lint
#pragma ident "@(#)evlog.c 1.8 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1991-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	evlog.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/param.h>
#include <time.h>
#include <stdarg.h>

#include "evlog.h"

#ifdef EVENT_LOG

#ifdef notdef

#ifdef EVENT_LOG
#include "evlog.h"
#endif	/* EVENT_LOG */
#ifdef EVENT_LOG
evlog_event();
#endif	/* EVENT_LOG */

#endif

int	_ev_log = 0;
char	logbuf[1024];		/* anybody logging can use this... */

static char    *equals =
"======================================================================";

/*
 * time/space variables ...
 */
static time_t   start = 0;
static time_t   end = 0;

static FILE    *fp;

/*
 * return a pointer to the currently open log file stream.
 * return stdout if no log file open for writing.
 */
FILE *
get_logfile_stream()
{
	if (fp != (FILE *) NULL)
	    return (fp);
	else
	    return (stdout);
}

int
evlog_init(char *fname)
{
	char	logfile[MAXPATHLEN];

	if (fname == (char *) NULL || *fname == '\0')
	    fname = "install.log";

	(void) sprintf(logfile, "./%s", fname);

	if ((fp = fopen(fname, "w")) == (FILE *) NULL) {
		(void) fprintf(stderr, "can't open event logfile: %s\n");
		return (0);		/* can't log, turn off logging */
	}
	start = time((time_t) NULL);

	(void) fprintf(fp, "%s\n=\tevent log started: %s%s\n\n",
	    equals, asctime(localtime(&start)), equals);

	(void) fprintf(fp, EVLOG_COLUMN_HEADING);

	return (1);
}

void
evlog_done(void)
{

	if (fp != (FILE *) NULL) {

		end = time((time_t) NULL);

		(void) fprintf(fp, "\n%s\n=\tevent log ended: %s%s\n\n",
		    equals, asctime(localtime(&end)), equals);

		(void) fflush(fp);
		(void) fclose(fp);
		fp = (FILE *) NULL;
	}
}

/*
 * log time elapsed since start.
 *
 * event `string' is contained in varargs
 */

/* VARARGS0 */
void
evlog_event(char *format, ...)
{
	va_list		args;

	register time_t now;
	register int    minutes;
	register int    elapsed, hrs, min, sec;

	if (_ev_log && fp != (FILE *) NULL) {
		now = time((time_t) NULL);

		hrs = min = sec = 0;
		elapsed = now - start;

		minutes = (int) (elapsed / 60);

		hrs = (int) (minutes / 60);
		min = (int) (minutes % 60);
		sec = (int) (elapsed % 60);

		/* record time stamp */
		(void) fprintf(fp, "\t%02d:%02d:%02d", hrs, min, sec);

		/* record event */
		va_start(args, format);
		(void) vfprintf(fp, format, args);
		va_end(args);

		(void) fprintf(fp, "\n");

	}
}

#endif	/* EVENT_LOG */
