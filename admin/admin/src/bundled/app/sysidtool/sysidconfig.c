/*
 * sys-config: System configuration application wrapper.
 *
 * Copyright (c) 1983-1993 Sun Minrosystems Inc.
 *
 */

#ident "@(#)sysidconfig.c 1.20 96/06/07 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <libintl.h>
#include <locale.h>
#include "findconf.h"
#include "sysidtool.h"
#include <unistd.h>

FILE *debugfp;

static int system_configured(int *sys_localep, char *termtype);

/*ARGSUSED*/
main(int argc, const char **argv)
{
	CFG_HANDLE	fp;
	int		c;
	AppsList	*applist;
	int		noexec = 0;
	char		termtype[MAX_TERM+2];
	char		tmpstr[MAX_TERM+11];
	int		locale_done;
	char		*test_input = (char *)0, *test_output = (char *)0;

	debugfp = open_log("sysidconfig");

	(void) lookup_locale();

	textdomain(TEXT_DOMAIN);

	init_cfg_err_msgs();

	/* publicize for exception handler */
	appname = strdup(basename((char *)argv[0]));
	if (!appname) {
		mod_cfg_err = MOD_CFG_MEMORY;
		fail_error();
	}

	if (getuid()) {
		mod_cfg_err = MOD_CFG_NOTROOT;
		fail_error();
	}

	while ((c = getopt(argc, (char **)argv, "a:r:lvb:O:i:I:")) != EOF) {
		switch (c) {
		case 'a': /* add an application */
			noexec = 1;
			fp = open_cfg_data("r+");
			if (mod_cfg_err) fail_error();
			add_cfg_app(fp, optarg);
			if (mod_cfg_err) fail_error();
			close_cfg_data(fp);
			if (mod_cfg_err) warn_error();
			break;
		case 'r': /* remove an application */
			noexec = 1;
			fp = open_cfg_data("r+");
			if (mod_cfg_err) fail_error();
			rem_cfg_app(fp, optarg);
			if (mod_cfg_err) fail_error();
			close_cfg_data(fp);
			if (mod_cfg_err) warn_error();
			break;
		case 'l': /* list applications */
			noexec = 1;
			fp = open_cfg_data("r");
			if (fp) { /* some apps to execute */
				if (mod_cfg_err) fail_error();
				applist = get_cfg_apps(fp);
				if (mod_cfg_err) fail_error();
				write_list(stdout, applist);
				free_list(applist);
				close_cfg_data(fp);
				if (mod_cfg_err) warn_error();
			}
			break;
		case 'v': /* verbose */
			verbosemode++;
			break;
		case 'b': /* basedir */
			basedir = strdup(optarg);
			break;
		case 'O':
			test_enable();
			sim_load(optarg);
			break;
		case 'i':
			test_input = optarg;
			break;
		case 'I':
			test_output = optarg;
			break;
		default: /* Illegal option */
			fprintf(stderr, "Illegal option %c\n", c);
			fprintf(stderr,
"Usage: %s [ -b <base> ] [ -a <app>] [ -r <app> ] [ -l ] [ -v ]\n");
			exit(1);
		}
	}
	if (testing && (test_input == (char *)0 || test_output == (char *)0)) {
		fprintf(stderr,
	    "%s: -O <name> must be accompanied by -I <name> and -i <name>\n",
			appname);
		exit(1);
	}

	if (testing) {
		if (sim_init(test_input, test_output) < 0) {
			fprintf(stderr,
			    "%s: Unable to initialize simulator\n",
			    appname);
			exit(1);
		}
	}
	/*
	 * First, assume if termtype is set in the
	 * environment that it is authoritative.
	 * Else, read the sysidtool state file to determine
	 * the termtype. If the termtype is set in the state file,
	 * put it into the environment
	 */
	termtype[0] = '\0';

	if (getenv("TERM") == NULL) {
		if (system_configured(&locale_done, termtype) == FALSE) {
			mod_cfg_err = MOD_CFG_NOTERM;
			fail_error();
		} else if (termtype[0] == '\0') {
			mod_cfg_err = MOD_CFG_NOTERM;
			fail_error();
		} else {
			(void) sprintf(tmpstr, "TERM=%s", termtype);
			(void) putenv(tmpstr);
		}
	} /* else TERM is set in the environment */
	if (!noexec) {
		execAllCfgApps((char *)argv[0], DO_CONFIG);
	}

	fprintf(debugfp, "sysidconfig end\nfree VM: %d\n\n", free_vm());
	return (0);
}

static int
system_configured(int *sys_localep, char *termtype)
{
	int sys_configured;
	int sys_bootparamed;
	int sys_networked;
	int sys_autobound;
	int sys_subnetted;
	int sys_passwd;
	int err_num;

	/*
	 * If there is no state file, then fail
	 * Else, read the termtype from the file and return it.
	 */

	if (get_state(&sys_configured, &sys_bootparamed, &sys_networked,
	    &sys_autobound, &sys_subnetted, &sys_passwd, sys_localep, termtype,
	    &err_num) == SUCCESS)
		return (TRUE);
	else
		return (FALSE);
	/* NOTREACHED */
}
