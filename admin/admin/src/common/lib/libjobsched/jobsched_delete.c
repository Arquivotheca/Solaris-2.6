/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_delete.c	1.4	95/07/26 SMI"


#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "jobsched_iface.h"


static const char	*at_cmd = "set -f; /usr/bin/at ";

/*
 * The editor environment variables have a sleep 1 hacked into them
 * here to workaround bug 1211756.  Once that bug is fixed, the cron_cmd
 * string should be replaced with
 *
 * static const char	*cron_cmd = "set -f; /usr/bin/env VISUAL=ed EDITOR=ed "
 *			    "/usr/bin/crontab -e ";
 *
 */

static const char	*cron_cmd = "set -f; "
			    /* CSTYLED */
			    "/usr/bin/env \"VISUAL=sleep 1; ed\" "
			    /* CSTYLED */
			    "\"EDITOR=sleep 1; ed\" "
			    "/usr/bin/crontab -e ";

/*
 * regular expression special characters that must be escaped when
 * building an ed command to delete a line from the crontab file.
 */

static const char	*re_special = ".*[]/^$\\";


int
jobsched_delete_at(JSAtArg *ap)
{

	char	cmd_buf[1024];


	if (ap == NULL || ap->at_job_id_key == NULL) {
		return (JOBSCHED_BAD_INPUT);
	}

	(void) strcpy(cmd_buf, at_cmd);
	(void) strcat(cmd_buf, "-r ");
	(void) strcat(cmd_buf, ap->at_job_id_key);

	(void) strcat(cmd_buf, " 1>/dev/null 2>&1");

	return (system(cmd_buf));
}


int
jobsched_delete_cron(JSCronArg *cp)
{

	char		cmd_buf[1024];
	char		*cbp;
	const char	*p;
	uid_t		uid;


	if (cp == NULL || cp->job_key == NULL ||
	    cp->minute_key == NULL || cp->hour_key == NULL ||
	    cp->month_key == NULL || cp->day_of_week_key == NULL) {

		return (JOBSCHED_BAD_INPUT);
	}

	/* check permission */

	if (cp->username_key != NULL && (uid = getuid()) != 0 &&
	    strcmp(getpwuid(uid)->pw_name, cp->username_key) != 0) {
		return (JOBSCHED_PERM_DENIED);
	}

	(void) strcpy(cmd_buf, cron_cmd);
	if (cp->username_key != NULL) {
		(void) strcat(cmd_buf, cp->username_key);
	}
	(void) strcat(cmd_buf, " 1>/dev/null 2>&1 <<end-of-cron\n");

	/* the guts */
	(void) strcat(cmd_buf, "/");

	(void) strcat(cmd_buf, "[ \t]*");
	if (*cp->minute_key == '*') {
		(void) strcat(cmd_buf, "\\");
	}
	(void) strcat(cmd_buf, cp->minute_key);

	(void) strcat(cmd_buf, "[ \t]*");
	if (*cp->hour_key == '*') {
		(void) strcat(cmd_buf, "\\");
	}
	(void) strcat(cmd_buf, cp->hour_key);

	(void) strcat(cmd_buf, "[ \t]*");
	if (*cp->day_key == '*') {
		(void) strcat(cmd_buf, "\\");
	}
	(void) strcat(cmd_buf, cp->day_key);

	(void) strcat(cmd_buf, "[ \t]*");
	if (*cp->month_key == '*') {
		(void) strcat(cmd_buf, "\\");
	}
	(void) strcat(cmd_buf, cp->month_key);

	(void) strcat(cmd_buf, "[ \t]*");
	if (*cp->day_of_week_key == '*') {
		(void) strcat(cmd_buf, "\\");
	}
	(void) strcat(cmd_buf, cp->day_of_week_key);

	(void) strcat(cmd_buf, "[ \t]*");

	if (strcspn(cp->job_key, re_special) == strlen(cp->job_key)) {
		/* no problematic RE characters in job */
		(void) strcat(cmd_buf, cp->job_key);
	} else {
		/* need to escape RE special chars when building ed command */
		cbp = cmd_buf + strlen(cmd_buf);
		for (p = cp->job_key; *p != '\0'; p++) {
			if (strchr(re_special, *p) != NULL) {
				*cbp++ = '\\';
				*cbp++ = *p;
			} else {
				*cbp++ = *p;
			}
		}

		*cbp = '\0';
	}

	(void) strcat(cmd_buf, "/d\nw\nq");

	(void) strcat(cmd_buf, "\nend-of-cron");

	return (system(cmd_buf));
}
