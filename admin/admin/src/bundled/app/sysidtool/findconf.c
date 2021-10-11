/*
 * findconf.c: This file contains routines to implement execution
 *				and management of config application list.
 *
 * Copyright (c) 1983-1993 Sun Minrosystems Inc.
 *
 */

/*LINTLIBRARY*/

#ident "@(#)findconf.c 1.16 94/05/23 SMI"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h> /* for malloc and system decl's */
#include <sys/wait.h>
#include <libgen.h>
#include <errno.h>
#include <varargs.h>
#include <sys/param.h> /* for MAXPATHLEN */
#include <time.h>
#include <libintl.h>
#include "findconf.h"
#include <unistd.h>

#define	WARN 1
#define	FAIL 2

/* Global variables initialized here */
char * appname = "Uninitialized";
int mod_cfg_err  = 0;
int mod_cfg_arg  = 0;
char * mod_cfg_errmsg[MOD_CFG_MAXERRMSGS+1];
int verbosemode = 0;
char * basedir = "";

#define	LOGFILE "%s/var/log/sysidconfig.log"
static char *
_logfile()
{
	static char buf[MAXPATHLEN];

	sprintf(buf, LOGFILE, basedir);
	return (buf);
}

#define	CFG_FILE "%s/etc/.sysidconfig.apps" /* a VERY private interface */
static char *
_cfg_filename()
{
	static char buf[MAXPATHLEN];

	sprintf(buf, CFG_FILE, basedir);
	return (buf);
}

static char *
_ltime()
{
	static char tbuf[80];
	time_t tm;

	tm = time((time_t *)NULL);
	strftime(tbuf, 80, "%c", localtime(&tm));
	return (tbuf);

}

static void
log(char * fmt, ...)
{
	va_list ap;
	FILE *logfile;

	va_start(ap);

	if (verbosemode)
		vfprintf(stderr, fmt, ap);

	logfile = fopen(_logfile(), "a");
	if (logfile) fchmod(fileno(logfile), S_IRWXU | S_IRWXG | S_IRWXO);
	if (!logfile) {
		fprintf(stderr, gettext("%s: Failure: Cannot open logfile\n"),
		    appname);
		exit(-1);
	}
	vfprintf(logfile, fmt, ap);
	fclose(logfile);
	va_end(ap);
}

static char *
_local_strdup(char * str)
{
	char *c = NULL;

	if ((c = strdup(str)) == NULL) {
		mod_cfg_err =  MOD_CFG_MEMORY;
		fail_error();
	}
	return (c);
}

#define	strdup(x) _local_strdup(x)


static void
doCfgApp(char * app, char * mode)
{
	pid_t pid;
	int stat_loc, sres;
	struct stat sbuf;

	sres = stat(app, &sbuf);
	if (sres == -1) {
		switch (errno) {
		case 0:
			break;
		case ENOENT: /* file does not exist */
			mod_cfg_err = MOD_CFG_NOAPP;
			warn_error();
			return;
		default:
			mod_cfg_err = MOD_CFG_STAT;
			mod_cfg_arg = sres;
			warn_error();
			break;
		}
	}

	if (verbosemode)
		printf("Executing [%s] with [%s]\n", app, mode);

	if ((pid = fork()) == 0) {
		execl(app, basename(app), mode, NULL);
		exit(MOD_CFG_EXECFAIL);
	}
	waitpid(pid, &stat_loc, 0);
	if (WIFEXITED(stat_loc) && WEXITSTATUS(stat_loc)) {
		mod_cfg_err = MOD_CFG_APPFAIL;
		mod_cfg_arg = WEXITSTATUS(stat_loc);
		warn_error();
	}

	if (WIFSIGNALED(stat_loc)) {
		mod_cfg_err = MOD_CFG_APPSIG;
		mod_cfg_arg = WTERMSIG(stat_loc);
		warn_error();
	}
}

static void
write_applist(AppsList *l)
{
	FILE *fp = fopen(_cfg_filename(), "w"); /* truncate it */

	while (l) {
		if (l->name) fprintf(fp, "%s\n", l->name);
		l = l->next;
	}
	fclose(fp);
}

static void
write_error(int mode)
{
	char ebuf[MAXPATHLEN];
	char *mstr;

	if (mode == WARN)
		mstr = gettext("Warning");
	else
		mstr = gettext("Failure");

	sprintf(ebuf, "%%s: %%s: %s\n", mod_cfg_errmsg[mod_cfg_err]);
	log(ebuf, appname, mstr, mod_cfg_arg);
}

/*
 * External Routines
 */

void
execAllCfgApps(char * app, char * mode)
{
	AppsList *cfgApps;
	CFG_HANDLE fp;
	char cmd[MAXPATHLEN];

	appname = app; /* copy to global appname for exceptions */

	log(gettext("Executing Configuration Applications at: %s\n"), _ltime());
	fp = open_cfg_data("r");
	if (!fp) {
		cfgApps = (AppsList *)NULL;
		log(gettext("No applications to execute...\n"));
	}
	else
		cfgApps = get_cfg_apps(fp);

	while (cfgApps) {
		sprintf(cmd, "%s%s", basedir, cfgApps->name);
		log(gettext("Executing config app: %s\n"), cfgApps->name);
		doCfgApp(cmd, mode);
		cfgApps = cfgApps->next;
	}

	free_list(cfgApps);
	close_cfg_data(fp);

	log(gettext(
	    "Completed Executing Configuration Applications at: %s\n"),
		_ltime());
}

CFG_HANDLE
open_cfg_data(char * amode)
{
	FILE *fp;
	struct stat sbuf;
	int sres;

	sres = stat(_cfg_filename(), &sbuf);
	if (sres == -1)
		switch (errno) {
		case 0:
		case ENOENT: /* file does not exist yet. */
			/* Opened read only; nothing to read */
			if (strcmp(amode, "r") == 0)
				return (NULL);
			amode = "w+";
			break;
		default:
			mod_cfg_err = MOD_CFG_STAT;
			mod_cfg_arg = sres;
			fail_error();
			break;
		}

	fp = fopen(_cfg_filename(), amode);
	if (!fp) {
		mod_cfg_err = MOD_CFG_FILEIO;
		mod_cfg_arg = errno;
		fail_error();
	}
	return ((CFG_HANDLE)fp);
}

AppsList *
get_cfg_apps(CFG_HANDLE h)
{
	FILE *fp = (FILE *)h;
	AppsList *apps = (AppsList *)NULL;
	AppsList *retv = (AppsList *)NULL;
	char rbuf[MAXPATHLEN];

	if (!h) {
		mod_cfg_err = MOD_CFG_BADHANDLE;
		mod_cfg_arg = 0;
		fail_error();
	}

	rewind(fp);

	while (fgets(rbuf, MAXPATHLEN, fp)) {
		if (apps) {
			apps->next = calloc(1, sizeof (AppsList));
			apps = apps->next;
			if (!apps) {
				mod_cfg_err = MOD_CFG_MEMORY;
				fail_error();
			}
		} else {
			apps = calloc(1, sizeof (AppsList));
			if (!apps) {
				mod_cfg_err = MOD_CFG_MEMORY;
				fail_error();
			}
			retv = apps; /* top of list */
		}
		rbuf[strlen(rbuf)-1] = 0; /* wipe \n */
		apps->name = strdup(rbuf);
	}
	return (retv);
}

void
add_cfg_app(CFG_HANDLE h, char *appname)
{
	FILE *fp = (FILE *)h;
	AppsList * apps = get_cfg_apps(fp);
	AppsList * p = (AppsList *)NULL, *top;

	if (appname[0] != '/') {
		mod_cfg_err = MOD_CFG_BADNAME;
		fail_error();
	}

	top = apps; /* first one */
	while (apps && apps->name) {
		if (strcmp(apps->name, appname) == 0) {
			mod_cfg_err = MOD_CFG_DUPFILE;
			fail_error();
		}
		p = apps; /* next to last */
		apps = apps->next;
	}

	/*
	 * got to the end of an existing list - did not find the app, or
	 * an empty entry
	 */
	if ((!apps) && p) {
		p->next = calloc(1, sizeof (AppsList));
		apps = p->next;
	}

	/* Manage the first app being added */
	if (!top) {
		top = calloc(1, sizeof (AppsList));
		apps = top;
	}

	apps->name = strdup(appname);
	write_applist(top);

	log("Added entry %s at %s\n", appname, _ltime());
}

void
rem_cfg_app(CFG_HANDLE h, char *appname)
{
	FILE *fp = (FILE *)h;
	AppsList * apps = get_cfg_apps(fp);
	AppsList *top;
	int found = 0;

	if (appname[0] != '/') {
		mod_cfg_err = MOD_CFG_BADNAME;
		fail_error();
	}

	top = apps;
	while (apps) {
		if (apps->name && strcmp(apps->name, appname) == 0) {
			free(apps->name);
			apps->name = NULL;
			found++;
			break;
		}
		apps = apps->next;
	}

	if (!found) {
		mod_cfg_err = MOD_CFG_NOFILE;
		warn_error();
	}

	write_applist(top);

	log("Removed entry %s at %s\n", appname, _ltime());
}

void
close_cfg_data(CFG_HANDLE h)
{
	FILE *fp = (FILE *)h;

	fclose(fp);
}

void
free_list(AppsList * l)
{
	AppsList *ln;

	while (l) {
		if (l->name) free(l->name);
		l->name = NULL; /* Meticulous to a fault */
		ln = l->next;
		free(l);
		l = ln;
	}
}

void
warn_error()
{
	write_error(WARN);
}

void
fail_error()
{
	verbosemode++;
	write_error(FAIL);
	exit(mod_cfg_err);
}

void
write_list(FILE * fp, AppsList * al)
{
	while (al) {
		if (al->name) fprintf(fp, "%s\n", al->name);
		al = al->next;
	}
}

void
init_cfg_err_msgs()
{
	int x = 0;
	mod_cfg_errmsg[x++] = gettext("Success");
	mod_cfg_errmsg[x++] = gettext("Entry Not Found");
	mod_cfg_errmsg[x++] = gettext("Duplicate Entry");
	mod_cfg_errmsg[x++] = gettext("Application Not Found");
	mod_cfg_errmsg[x++] = gettext("File I/O error %d");
	mod_cfg_errmsg[x++] = gettext("Not Full Pathname");
	mod_cfg_errmsg[x++] = gettext("Exec of application failed");
	mod_cfg_errmsg[x++] = gettext("Application returned failure code %d");
	mod_cfg_errmsg[x++] = gettext("Application exited on signal %d");
	mod_cfg_errmsg[x++] = gettext("Cannot stat data file");
	mod_cfg_errmsg[x++] = gettext("Bad Handle passed to function");
	mod_cfg_errmsg[x++] = gettext("Memory Error");
	mod_cfg_errmsg[x++] = gettext("Must be run as superuser");
	mod_cfg_errmsg[x++] = gettext("Unable to determine terminal type");
	mod_cfg_errmsg[x] = NULL;
}
