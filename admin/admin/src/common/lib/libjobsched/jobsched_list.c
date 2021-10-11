/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_list.c	1.11	95/08/01 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "jobsched_iface.h"


static const char	*at_list_cmd = "set -f; /usr/bin/at -l";
static const char	*cron_list_cmd = "set -f; /usr/bin/crontab -l";
static const char	*at_spool_dir = "/var/spool/cron/atjobs";


int
jobsched_list_at(JSAtArg **app)
{

	JSAtArg		*curp;
	JSAtArg		get_at;
	DIR		*atdir;
	FILE		*atpipe;
	int		cnt = 0;
	int		num_atjobs;
	char		buf[256];
	uid_t		uid;
	boolean_t	is_root;
	char		username[256];
	char		at_job_name[256];
	time_t		t;
	struct tm	*tt;
	char		*p;


	if (app == NULL) {
		return (JOBSCHED_BAD_INPUT);
	}

	/*
	 * I realize that this counts . and .. as well as all of the
	 * jobs for EVERY queue, and we're only going to return the
	 * default queue (queue a), but I'd rather allocate a bit
	 * too much -- it'll get free'd later anyway.
	 */

	if ((atdir = opendir(at_spool_dir)) == NULL) {
		return (JOBSCHED_FAILURE);
	}
	while (readdir(atdir) != NULL) {
		cnt++;
	}
	(void) closedir(atdir);

	*app = (JSAtArg *)malloc((unsigned)(cnt * sizeof (JSAtArg)));

	if (*app == NULL) {
		return (JOBSCHED_MALLOC_ERR);
	}

	if ((uid = getuid()) == 0) {
		is_root = B_TRUE;
	} else {
		is_root = B_FALSE;
		(void) strcpy(username, getpwuid(uid)->pw_name);
	}

	if ((atpipe = popen(at_list_cmd, "r")) == NULL) {
		free((void *)*app);
		*app = NULL;
		return (JOBSCHED_FAILURE);
	}

	curp = *app;

	num_atjobs = 0;
	while (fgets(buf, sizeof (buf), atpipe) != NULL && num_atjobs < cnt) {

		if (is_root == B_TRUE) {
			(void) sscanf(buf, "%*s%*s%s%s", username, at_job_name);
		} else {
			(void) sscanf(buf, "%s", at_job_name);
		}

		curp->owner = strdup(username);
		curp->at_job_id = strdup(at_job_name);

		get_at.at_job_id_key = at_job_name;
		if (jobsched_get_at(&get_at) == JOBSCHED_SUCCESS) {
			curp->job = strdup(get_at.job);
			jobsched_free_at(&get_at);
		} else {
			curp->job = NULL;
		}

		/*
		 * The job name (filename) consists of the date, a dot,
		 * and the queue that the job is in.  Remove the queue
		 * and convert the date to the right format.
		 */

		if ((p = strchr(buf, '.')) != NULL) {
			*p = '\0';
		}

		t = atol(buf);

		tt = localtime(&t);

		curp->year = tt->tm_year;
		curp->month = tt->tm_mon + 1;
		curp->day_of_month = tt->tm_mday;
		curp->hour = tt->tm_hour;
		curp->minute = tt->tm_min;
		curp->sec = tt->tm_sec;

		curp++;
		num_atjobs++;
	}

	(void) pclose(atpipe);

	return (num_atjobs);
}


int
jobsched_get_at(JSAtArg *ap)
{

	FILE		*fp;
	char		at_filename[PATH_MAX + 1];
	char		buf[1024];
	char		shell_env[256];
	uid_t		uid;
	struct stat	st;
	char		*p;
	time_t		t;
	struct tm	*tt;


	if (ap == NULL || ap->at_job_id_key == NULL) {
		ap->at_job_id = NULL;
		ap->job = NULL;
		ap->owner = NULL;
		return (JOBSCHED_BAD_INPUT);
	}

	(void) sprintf(at_filename, "%s/%s", at_spool_dir, ap->at_job_id_key);
	if (stat(at_filename, &st) != 0) {
		ap->at_job_id = NULL;
		ap->job = NULL;
		ap->owner = NULL;
		return (JOBSCHED_NOT_FOUND);
	}

	if ((uid = getuid()) != 0 && uid != st.st_uid) {
		ap->at_job_id = NULL;
		ap->job = NULL;
		ap->owner = NULL;
		return (JOBSCHED_PERM_DENIED);
	}

	if ((fp = fopen(at_filename, "r")) == NULL) {
		ap->at_job_id = NULL;
		ap->job = NULL;
		ap->owner = NULL;
		return (JOBSCHED_FAILURE);
	}

	/* read ": at job" line */
	(void) fgets(buf, sizeof (buf), fp);

	/* read ": jobname: " line */
	(void) fgets(buf, sizeof (buf), fp);
	/* remove newline */
	buf[strlen(buf) - 1] = '\0';
	p = buf + 11;
	if (strcmp(p, "stdin") == 0) {
		ap->job_is_filename_p = B_FALSE;
	} else {
		ap->job_is_filename_p = B_TRUE;
		ap->job = strdup(p);
	}

	/* read ": notify by mail: " line */
	(void) fgets(buf, sizeof (buf), fp);
	/* remove newline */
	buf[strlen(buf) - 1] = '\0';
	p = buf + 18;
	if (strcmp(p, "no") == 0) {
		ap->send_at_mail_p = B_FALSE;
	} else {
		ap->send_at_mail_p = B_TRUE;
	}

	while (fgets(buf, sizeof (buf), fp) != NULL) {
		if (strncmp(buf, "export", 6) == 0) {
			if (strncmp(buf + 7, "SHELL", 5) == 0) {
				/* export SHELL; SHELL='<shell>' */
				(void) strcpy(shell_env, buf + 21);
				/* remove trailing ' and \n */
				shell_env[strlen(shell_env) - 2] = '\0';
			}
			continue;
		} else {
			/* done reading environ, break out */
			break;
		}
	}

	/*
	 * we're at the line that says
	 * "<shell> << '... the rest of this line is shell input'"
	 * after we read that line and process the shell type,
	 * well read a couple more lines of junk -- a #ident string,
	 * a cd command, a umask, and if sh or ksh, a ulimit command.
	 */

	if (strncmp(buf, "$SHELL", 6) == 0) {
		if (strcmp(shell_env, "/bin/sh") == 0) {
			ap->shell = js_sh;
		} else if (strcmp(shell_env, "/bin/csh") == 0) {
			ap->shell = js_csh;
		} else {
			/* assume no funky shell, 'cause we can't handle it. */
			ap->shell = js_ksh;
		}
	} else {
		if (strncmp(buf, "/bin/sh", 7) == 0) {
			ap->shell = js_sh;
		} else if (strncmp(buf, "/bin/csh", 8) == 0) {
			ap->shell = js_csh;
		} else {
			ap->shell = js_ksh;
		}
	}

	if (ap->job_is_filename_p == B_TRUE) {
		/*
		 * we're done, we already have the filename, so just
		 * close the file and return.
		 */
		(void) fclose(fp);
		return (JOBSCHED_SUCCESS);
	}

	/* ident */
	(void) fgets(buf, sizeof (buf), fp);
	/* cd */
	(void) fgets(buf, sizeof (buf), fp);
	/* umask */
	(void) fgets(buf, sizeof (buf), fp);
	if (ap->shell == js_sh || ap->shell == js_ksh) {
		/* ulimit */
		(void) fgets(buf, sizeof (buf), fp);
	}

	/*
	 * To avoid a bunch of malloc/realloc cluttering up the code
	 * here, just malloc enough space to read the entire file,
	 * even though we're just going to read a little bit of it.
	 */

	if ((ap->job = (char *)malloc((unsigned)st.st_size)) == NULL) {
		(void) fclose(fp);
		free((void *)ap->job);
		ap->at_job_id = NULL;
		ap->job = NULL;
		ap->owner = NULL;
		return (JOBSCHED_MALLOC_ERR);
	}

	/* read the rest of the lines into the buf */
	ap->job[0] = '\0';
	while (fgets(buf, sizeof (buf), fp) != NULL) {
		(void) strcat(ap->job, buf);
	}

	/*
	 * There will be two extra newline characters, one from the
	 * final fgets and one that is supplied by at when it creates
	 * the job file.  Nuke 'em.
	 */

	ap->job[strlen(ap->job) - 2] = '\0';

	(void) fclose(fp);

	/*
	 * last few things -- copy the key into the id, owner from
	 * the statbuf, date from filename
	 */

	ap->at_job_id = strdup(ap->at_job_id_key);
	ap->owner = strdup(getpwuid(st.st_uid)->pw_name);

	(void) strcpy(buf, ap->at_job_id_key);
	if ((p = strchr(buf, '.')) != NULL) {
		*p = '\0';
	}

	t = atol(buf);

	tt = localtime(&t);

	ap->year = tt->tm_year;
	ap->month = tt->tm_mon + 1;
	ap->day_of_month = tt->tm_mday;
	ap->hour = tt->tm_hour;
	ap->minute = tt->tm_min;
	ap->sec = tt->tm_sec;

	return (JOBSCHED_SUCCESS);
}


void
jobsched_free_at(JSAtArg *ap)
{

	if (ap == NULL) {
		return;
	}

	if (ap->job != NULL) {
		free((void *)ap->job);
	}
	if (ap->owner != NULL) {
		free((void *)ap->owner);
	}
	if (ap->at_job_id != NULL) {
		free((void *)ap->at_job_id);
	}
}


void
jobsched_free_at_list(JSAtArg *ap, int cnt)
{

	int	i;


	if (ap == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		jobsched_free_at(ap + i);
	}

	free((void *)ap);
}


int
jobsched_list_cron(JSCronArg **cpp, const char *username)
{

	uid_t		uid;
	FILE		*cronpipe;
	JSCronArg	*curp;
	int		cnt;
	int		num_cron;
	char		*p;
	char		cmd[256];
	char		buf[256];
	char		tmp[1024];


	if (cpp == NULL) {
		return (JOBSCHED_BAD_INPUT);
	}

	/* check permission */

	if (username != NULL && (uid = getuid()) != 0 &&
	    strcmp(getpwuid(uid)->pw_name, username) != 0) {
		return (JOBSCHED_PERM_DENIED);
	}

	/* First get a count of the number of lines in the crontab */

	if (username != NULL) {
		(void) sprintf(cmd, "%s %s | /usr/bin/wc",
		    cron_list_cmd, username);
	} else {
		(void) sprintf(cmd, "%s | /usr/bin/wc", cron_list_cmd);
	}

	if ((cronpipe = popen(cmd, "r")) == NULL) {
		return (JOBSCHED_FAILURE);
	}

	(void) fgets(buf, sizeof (buf), cronpipe);
	num_cron = atol(buf);

	/*
	 * should only be one line of output, but make sure all
	 * output is gobbled up to avoid "Broken Pipe" problems.
	 */

	while (fgets(buf, sizeof (buf), cronpipe) != NULL)
		;

	(void) pclose(cronpipe);

	*cpp = (JSCronArg *)malloc((unsigned)(num_cron * sizeof (JSCronArg)));
	if (*cpp == NULL) {
		return (JOBSCHED_MALLOC_ERR);
	}

	/*
	 * Now read the entries from the crontab; there is a very remote
	 * chance that the crontab file has been modified (grew) since we
	 * counted the entries, so make sure that we don't overflow
	 * the space that we malloc'd.
	 */

	(void) strcpy(cmd, cron_list_cmd);
	if (username != NULL) {
		(void) strcat(cmd, " ");
		(void) strcat(cmd, username);
	}

	if ((cronpipe = popen(cmd, "r")) == NULL) {
		free((void *)*cpp);
		*cpp = NULL;
		return (JOBSCHED_FAILURE);
	}

	curp = *cpp;

	cnt = 0;
	while (fgets(buf, sizeof (buf), cronpipe) != NULL && cnt < num_cron) {

		if (buf[0] == '#') {
			/* comment line, don't parse */
			continue;
		}

		if (username != NULL) {
			curp->username = strdup(username);
		} else {
			curp->username = NULL;
		}

		p = strtok(buf, " \t");
		curp->minute = strdup(p);

		p = strtok(NULL, " \t");
		curp->hour = strdup(p);

		p = strtok(NULL, " \t");
		curp->day = strdup(p);

		p = strtok(NULL, " \t");
		curp->month = strdup(p);

		p = strtok(NULL, " \t");
		curp->day_of_week = strdup(p);

		tmp[0] = '\0';
		while ((p = strtok(NULL, " \t")) != NULL) {
			(void) strcat(tmp, p);
			(void) strcat(tmp, " ");
		}
		/*
		 * The fgets returned a newline, and the while loop added
		 * an extra space character; kill them.
		 */
		tmp[strlen(tmp) - 2] = '\0';

		curp->job = strdup(tmp);

		curp++;
		cnt++;
	}

	(void) pclose(cronpipe);

	return (cnt);
}


int
jobsched_get_cron(JSCronArg *cp)
{

	int		i;
	JSCronArg	*list_cp;
	int		cnt;


	cp->minute = NULL;
	cp->hour = NULL;
	cp->day = NULL;
	cp->month = NULL;
	cp->day_of_week = NULL;
	cp->job = NULL;

	/*
	 * This function isn't terribly useful, since every field is needed
	 * to construct a "key" for crontab entry lookup.  The utility of
	 * this function is that it does do a lookup, so will return a
	 * "notfound" indication if the entry doesn't exist.
	 */

	cnt = jobsched_list_cron(&list_cp, cp->username_key);

	if (cnt < 0) {
		return (cnt);
	}

	for (i = 0; i < cnt; i++) {

		if (strcmp(cp->minute_key, list_cp[i].minute) == 0 &&
		    strcmp(cp->hour_key, list_cp[i].hour) == 0 &&
		    strcmp(cp->day_key, list_cp[i].day) == 0 &&
		    strcmp(cp->month_key, list_cp[i].month) == 0 &&
		    strcmp(cp->day_of_week_key, list_cp[i].day_of_week) == 0 &&
		    strcmp(cp->job_key, list_cp[i].job) == 0) {

			if (list_cp[i].username != NULL) {
				cp->username = strdup(list_cp[i].username);
			} else {
				cp->username = NULL;
			}
			cp->minute = strdup(list_cp[i].minute);
			cp->hour = strdup(list_cp[i].hour);
			cp->day = strdup(list_cp[i].day);
			cp->month = strdup(list_cp[i].month);
			cp->day_of_week = strdup(list_cp[i].day_of_week);
			cp->job = strdup(list_cp[i].job);

			return (JOBSCHED_SUCCESS);
		}
	}

	return (JOBSCHED_NOT_FOUND);
}


void
jobsched_free_cron(JSCronArg *cp)
{

	if (cp == NULL) {
		return;
	}

	if (cp->username != NULL) {
		free((void *)cp->username);
	}
	if (cp->minute != NULL) {
		free((void *)cp->minute);
	}
	if (cp->hour != NULL) {
		free((void *)cp->hour);
	}
	if (cp->day != NULL) {
		free((void *)cp->day);
	}
	if (cp->month != NULL) {
		free((void *)cp->month);
	}
	if (cp->day_of_week != NULL) {
		free((void *)cp->day_of_week);
	}
	if (cp->job != NULL) {
		free((void *)cp->job);
	}
}


void
jobsched_free_cron_list(JSCronArg *cp, int cnt)
{

	int	i;


	if (cp == NULL) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		jobsched_free_cron(cp + i);
	}

	free((void *)cp);
}
