/*
 * A simple test driver for libjobsched.  Use it as a regression test,
 * or a utility program for quickly getting into dbx to poke around
 * at changes to libjobsched -- whatever.
 *
 * cc -g -o test test.c objs/sparc/libjobsched.a
 *  - or -
 * cc -g -o test test.c -L`pwd`/pics/sparc -R`pwd`/pics/sparc -ljobsched
 */

#pragma	ident	"@(#)test.c	1.3	95/06/29 SMI"

#include <stdio.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>
#include "jobsched_iface.h"


int
test_at_success(void)
{
	int		cnt;
	JSAtArg		at;
	JSAtArg		*at_p;
	DIR		*dp;
	FILE		*fp;
	char		*old_job_p;
	char		*new_job_p;
	char		buf[1024];
	struct stat	statbuf;
	int		failcnt = 0;


	/* ---------------------------------------------------------------- */

	/* add a job */

	at.job = "/bin/true";
	at.job_is_filename_p = B_FALSE;
	at.shell = js_sh;
	at.send_at_mail_p = B_FALSE;
	at.year = 99;
	at.month = 12;
	at.day_of_month = 31;
	at.hour = 23;
	at.min = 59;
	at.sec = 58;

	if (jobsched_add_at(&at, &new_job_p) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_add_at()\n");
		failcnt++;
	}

	/* make sure the job got added */

	sprintf(buf, "/var/spool/cron/atjobs/%s", new_job_p);
	if (stat(buf, &statbuf)) {
		fprintf(stderr, "FAIL verifying jobsched_add_at()\n");
		failcnt++;
	}

	/* ---------------------------------------------------------------- */

	/* modify the job */

	old_job_p = new_job_p;

	at.at_job_id_key = old_job_p;
	at.job = "/bin/foo\n/bin/bar";

	if (jobsched_modify_at(&at, &new_job_p) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_modify_at()\n");
		failcnt++;
	}

	/* make sure the old job got removed, new one got added */

	sprintf(buf, "/var/spool/cron/atjobs/%s", old_job_p);
	if (! stat(buf, &statbuf)) {
		fprintf(stderr, "FAIL verifying jobsched_modify_at()\n");
		failcnt++;
	}

	sprintf(buf, "/var/spool/cron/atjobs/%s", new_job_p);
	if (stat(buf, &statbuf)) {
		fprintf(stderr, "FAIL verifying jobsched_modify_at()\n");
		failcnt++;
	}

	/* ---------------------------------------------------------------- */

	/* list at jobs */

	(void) jobsched_list_at(&at_p);

	/* ---------------------------------------------------------------- */

	/* get the job */

	memset((void *)&at, 0, sizeof (at));
	at.at_job_id_key = new_job_p;

	if (jobsched_get_at(&at) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_get_at()\n");
		failcnt++;
	} else {

		/*
		 * make sure return stuff is correct -- seconds will have
		 * ticked up one from the modify.
		 */

		if (strcmp(at.job, "/bin/foo\n/bin/bar") != 0 ||
		    at.job_is_filename_p != B_FALSE ||
		    at.shell != js_sh ||
		    at.send_at_mail_p != B_FALSE ||
		    at.year != 99 ||
		    at.month != 12 ||
		    at.day_of_month != 31 ||
		    at.hour != 23 ||
		    at.min != 59 ||
		    at.sec != 59) {
			fprintf(stderr, "FAIL verifying jobsched_get_at()\n");
			failcnt++;
		}
	}

	/* ---------------------------------------------------------------- */

	/* delete the job */

	at.at_job_id_key = new_job_p;

	if (jobsched_delete_at(&at) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_delete_at()\n");
		failcnt++;
	}

	/* make sure the job got removed */

	sprintf(buf, "/var/spool/cron/atjobs/%s", old_job_p);
	if (! stat(buf, &statbuf)) {
		fprintf(stderr, "FAIL verifying jobsched_delete_at()\n");
		failcnt++;
	}

	free((void *)old_job_p);
	free((void *)new_job_p);

	/* ---------------------------------------------------------------- */

	return (failcnt);
}

int
test_at_failure(void)
{

	int	failcnt = 0;


	return (failcnt);
}

int
test_cron_success(void)
{

	int		cnt;
	JSCronArg	cron;
	JSCronArg	*cron_p;
	char		buf[1024];
	int		failcnt = 0;


	/* ---------------------------------------------------------------- */

	/* add a cron job */

	cron.username = NULL;
	cron.minute = "0";
	cron.hour = "0,12";
	cron.day = "*";
	cron.month = "*";
	cron.day_of_week = "*";
	cron.job = "/bin/foo";

	if (jobsched_add_cron(&cron) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_add_cron()\n");
		failcnt++;
	}

	/* Can't grep the crontab files unless root, so only verify if uid 0 */

	if (getuid() == 0) {
		if (system("set -f ; grep '0 0,12 * * * /bin/foo' /var/spool/cron/crontabs/root 1>/dev/null 2>&1") != 0) {
			fprintf(stderr, "FAIL verifying jobsched_add_cron()\n");
			failcnt++;
		}
	}

	/* ---------------------------------------------------------------- */

	/* modify the job */

	cron.username_key = NULL;
	cron.minute_key = "0";
	cron.hour_key = "0,12";
	cron.day_key = "*";
	cron.month_key = "*";
	cron.day_of_week_key = "*";
	cron.job_key = "/bin/foo";

	cron.month = "1-6";
	cron.job = "/bin/bar";

	if (jobsched_modify_cron(&cron) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_modify_cron()\n");
		failcnt++;
	}

	/* Can't grep the crontab files unless root, so only verify if uid 0 */

	if (getuid() == 0) {
		if (system("set -f ; grep '0 0,12 * 1-6 * /bin/bar' /var/spool/cron/crontabs/root 1>/dev/null 2>&1") != 0) {
			fprintf(stderr, "FAIL verifying jobsched_modify_cron()\n");
			failcnt++;
		}
	}

	/* ---------------------------------------------------------------- */

	/* get the job */

	cron.username_key = NULL;
	cron.minute_key = "0";
	cron.hour_key = "0,12";
	cron.day_key = "*";
	cron.month_key = "1-6";
	cron.day_of_week_key = "*";
	cron.job_key = "/bin/bar";

	if (jobsched_get_cron(&cron) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_get_cron()\n");
		failcnt++;
	} else {
		if ((cron.username_key && cron.username &&
		     strcmp(cron.username_key, cron.username) != 0) ||
		    strcmp(cron.minute_key, cron.minute) != 0 ||
		    strcmp(cron.hour_key, cron.hour) != 0 ||
		    strcmp(cron.day_key, cron.day) != 0 ||
		    strcmp(cron.month_key, cron.month) != 0 ||
		    strcmp(cron.day_of_week_key, cron.day_of_week) != 0 ||
		    strcmp(cron.job_key, cron.job) != 0) {
			fprintf(stderr, "FAIL verifying jobsched_get_cron()\n");
			failcnt++;
		}
	}

	/* ---------------------------------------------------------------- */

	/* delete the job */

	if (jobsched_delete_cron(&cron) != JOBSCHED_SUCCESS) {
		fprintf(stderr, "FAIL jobsched_delete_cron()\n");
		failcnt++;
	}

	/* Can't grep the crontab files unless root, so only verify if uid 0 */

	if (getuid() == 0) {
		if (system("set -f ; grep '0 0,12 * 1-6 * /bin/bar' /var/spool/cron/crontabs/root 1>/dev/null 2>&1") == 0) {
			fprintf(stderr, "FAIL verifying jobsched_delete_cron()\n");
			failcnt++;
		}
	}

	/* ---------------------------------------------------------------- */

	return (failcnt);
}

int
test_cron_failure(void)
{

	int	failcnt = 0;


	return (failcnt);
}


int
main(int argc, char *argv[])
{

	int	failcnt = 0;


	failcnt += test_at_success();
	failcnt += test_at_failure();
	failcnt += test_cron_success();
	failcnt += test_cron_failure();

	/* final report */

	if (failcnt) {
		fprintf(stderr, "\nTOTAL FAILURES: %d\n", failcnt);
	}

	exit(failcnt);
}
