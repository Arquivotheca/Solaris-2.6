/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_add.c	1.7	95/07/21 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
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


int
jobsched_add_at(JSAtArg *ap, char **ret_job_id)
{

	int		status;
	char		cmd_buf[1024];
	struct tm	tt;
	char		touch_time_buf[32];
	time_t		sched_time;
	time_t		now_time;
	FILE		*pp;
	char		buf[256];
	char		tmp[16];


	if (ap == NULL || ap->job == NULL) {
		return (JOBSCHED_BAD_INPUT);
	}

	tt.tm_year = ap->year;
	if (tt.tm_year > 1900) {
		tt.tm_year -= 1900;
	}
	tt.tm_mon = ap->month - 1;
	tt.tm_mday = ap->day_of_month;
	tt.tm_hour = ap->hour;
	tt.tm_min = ap->minute;
	tt.tm_sec = ap->sec;
	tt.tm_isdst = -1;

	sched_time = mktime(&tt);
	now_time = time((time_t *)NULL);

	if (sched_time == -1 || now_time == -1) {
		return (JOBSCHED_FAILURE);
	}

	if (sched_time < now_time) {
		return (JOBSCHED_SCHEDULING_IN_THE_PAST);
	}

	(void) strcpy(cmd_buf, at_cmd);

	switch (ap->shell) {
	case js_csh:
		(void) strcat(cmd_buf, "-c ");
		break;
	case js_ksh:
		(void) strcat(cmd_buf, "-k ");
		break;
	case js_sh:
	default:
		(void) strcat(cmd_buf, "-s ");
		break;
	}

	if (ap->send_at_mail_p != B_FALSE) {
		(void) strcat(cmd_buf, "-m ");
	}

	/* at uses same time format as touch(1) */
	(void) sprintf(touch_time_buf, "%2.2d%2.2d%2.2d%2.2d%2.2d.%2.2d",
	    tt.tm_year, tt.tm_mon + 1, tt.tm_mday,
	    tt.tm_hour, tt.tm_min, tt.tm_sec);

	(void) strcat(cmd_buf, "-t ");
	(void) strcat(cmd_buf, touch_time_buf);

	if (ap->job_is_filename_p != B_FALSE) {
		(void) strcat(cmd_buf, " -f ");
		(void) strcat(cmd_buf, ap->job);
		if (ret_job_id == NULL) {
			(void) strcat(cmd_buf, " 1>/dev/null 2>&1");
		} else {
			/* capture both stderr and stdout */
			(void) strcat(cmd_buf, " 2>&1");
		}
	} else {
		if (ret_job_id == NULL) {
			(void) strcat(cmd_buf, " 1>/dev/null 2>&1");
		} else {
			(void) strcat(cmd_buf, " 2>&1");
		}
		(void) strcat(cmd_buf, " <<end-of-at\n");
		(void) strcat(cmd_buf, ap->job);
		(void) strcat(cmd_buf, "\nend-of-at");
	}

	if (ret_job_id != NULL) {

		/*
		 * Do a popen so we can read the job id from the at output;
		 * format of the output line we're interested in is
		 * "job nnnnnnnnn.x at ...", where nnnnnnnnn is the
		 * time that the scheduled job will be run, and x is
		 * the queue that the job is in.  Concatenated with
		 * a dot separator they make the job id.
		 */

		if ((pp = popen(cmd_buf, "r")) != NULL) {
			while (fgets(buf, sizeof (buf), pp) != NULL) {
				if (strlen(buf) > 18 &&
				    strncmp(buf, "job", 3) == 0 &&
				    strncmp(buf + 16, "at", 2) == 0) {

					(void) strncpy(tmp, buf + 4, 11);
					tmp[11] = '\0';
					*ret_job_id = strdup(tmp);

					break;
				}
			}
			/* gobble up remaining output to avoid Broken Pipe */
			while (fgets(buf, sizeof (buf), pp) != NULL)
				;
		}
		status = pclose(pp);
	} else {
		status = system(cmd_buf);
	}

	return (status);
}


int
jobsched_add_cron(JSCronArg *cp)
{

	char	cmd_buf[1024];
	uid_t	uid;


	if (cp == NULL || cp->job == NULL ||
	    cp->minute == NULL || cp->hour == NULL || cp->month == NULL ||
	    cp->day_of_week == NULL) {

		return (JOBSCHED_BAD_INPUT);
	}

	/* check permission */

	if (cp->username != NULL && (uid = getuid()) != 0 &&
	    strcmp(getpwuid(uid)->pw_name, cp->username) != 0) {
		return (JOBSCHED_PERM_DENIED);
	}

	(void) strcpy(cmd_buf, cron_cmd);
	if (cp->username != NULL) {
		(void) strcat(cmd_buf, cp->username);
	}
	(void) strcat(cmd_buf, " 1>/dev/null 2>&1 <<end-of-cron\n");

	/* the guts */
	(void) strcat(cmd_buf, "$\na\n");
	(void) strcat(cmd_buf, cp->minute);
	(void) strcat(cmd_buf, " ");
	(void) strcat(cmd_buf, cp->hour);
	(void) strcat(cmd_buf, " ");
	(void) strcat(cmd_buf, cp->day);
	(void) strcat(cmd_buf, " ");
	(void) strcat(cmd_buf, cp->month);
	(void) strcat(cmd_buf, " ");
	(void) strcat(cmd_buf, cp->day_of_week);
	(void) strcat(cmd_buf, " ");
	(void) strcat(cmd_buf, cp->job);
	(void) strcat(cmd_buf, "\n.\nw\nq");

	(void) strcat(cmd_buf, "\nend-of-cron");

	return (system(cmd_buf));
}
