#ifndef lint
#pragma ident "@(#)app_params.c 1.6 96/07/16 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_params.c
 * Group:	libspmiapp
 * Description:
 *	Module for handling the parsing of common install
 *	command line options.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "spmicommon_api.h"
#include "spmisoft_api.h"
#include "spmistore_api.h"
#include "spmiapp_api.h"

#include "app_strings.h"

/* externs used for getopt */
extern  char    *optarg;
extern	int	optind;
extern	int	opterr;

/* static definitions */

/* BE getopt args string */
static char _app_common_args[] = "c:d:hDx:";

/*
 * Function: ParamsGetCommonArgs
 * Description:
 *	Tell what the getopt(3) args string for the BE common
 *	arguments are.
 * Scope:	PUBLIC
 * Parameters:  none
 * Return:	char *
 *		getopt(3) args string for the BE common arguments
 * Globals:	_app_common_args - [RO][MODULE]
 * Notes:
 */
char *
ParamsGetCommonArgs(void)
{
	return (_app_common_args);
}

/*
 * Function:
 * Description:
 *	Parse the common install application command line options.
 *	Non-common command line options should already have been
 *	parsed out by the main app (e.g. -M, -u in installtool).
 *	However, since these non-common options may not have been
 *	removed from argc, argv prior to being sent to this routine,
 *	this routine is written to allow these to pass through
 *	getopt(3) without complaining.
 * Scope:	PUBLIC
 * Parameters:
 *	argc, argv - [RO]
 *		standard C argc, argv command line params to parse
 *	param_usage - [RO]
 *		information on app level options and usage strings.
 *	profile - [RW]
 *		structure used to store parsed param values into
 *
 * Return:	the optindex value into argv last set by getopt().
 * Globals:	none
 * Notes:
 */
int
ParamsParseCommonArgs(int argc, char **argv,
	ParamUsage *param_usage, Profile *profile)
{
	/* options string for getopt */
	char options[32];
	int c;

	/* make the full (FE/BE) getopt string */
	(void) strcpy(options, ParamsGetCommonArgs());
	if (param_usage->app_args)
		(void) strcat(options, param_usage->app_args);

	/*
	 * make sure getopt vars are set the way we want in case
	 * a FE app set them differently.
	 */
	opterr = 1;
	optind = 1;

	/* actually parse the arguments */
	while ((c = getopt(argc, argv, options)) != EOF) {
		switch (c) {

		/* PUBLIC OPTIONS */
		case 'c': /* cdrom base dir */
			MEDIANAME(profile) = xstrdup(optarg);
			break;
		case 'd': /* disk file */
			(void) SetSimulation(SIM_EXECUTE, 1);
			(void) SetSimulation(SIM_SYSDISK, 1);
			/*
			 * When we can simulate mounting disks, SIM_SYSSOFT
			 * should possibly be set by a -S flag.
			 */
			(void) SetSimulation(SIM_SYSSOFT, 1);
			DISKFILE(profile) = xstrdup(optarg);
			break;
		case 'h': /* usage */
			ParamsPrintUsage(param_usage);
			exit(EXIT_INSTALL_FAILURE);
			break;
		case 'D': /* simulation with local disks */
			(void) SetSimulation(SIM_EXECUTE, 1);
			break;

		/* PRIVATE OPTIONS */

		/*
		 * x should go away eventually in deference to
		 * forthcoming standardized tracing/debugging interfaces.
		 */
		case 'x':
			(void) set_trace_level(atoi(optarg));
			break;

		/* bad options or usage request */
		case '?':
			ParamsPrintUsage(param_usage);
			exit(EXIT_INSTALL_FAILURE);
			break;

		/*
		 * Ignore any options that fall through to here -
		 * i.e. Fe args should fall through here and
		 * be ignored since they should have already been
		 * handled elsewhere.
		 */
		default:
			break;
		}
	}

	return (optind);
}

/*
 * Function: ParamsValidateCommonArgs
 * Description:
 *	Provide param defaults if necessary and validate params.
 * Scope:	PUBLIC
 * Parameters:
 *		profile - [RW]
 * Return:	[void]
 * Globals:	none
 * Notes:
 *	In the future this should be expanded - I would stick in disk vtoc
 *	file validation and such as well, but there is enough difference
 *	between pfinstall and the ui apps right now, that I didn't tackle this
 *	just yet...
 */
void
ParamsValidateCommonArgs(Profile *profile)
{

	/*
	 * cdrom base dir
	 */
	if (MEDIANAME(profile) == NULL)
		MEDIANAME(profile) = "/cdrom";


}

/*
 * Function: ParamsPrintUsage
 * Description:
 *	Print a usage line for the install app in question.
 * Scope:	PUBLIC
 * Parameters:
 *		param_usage -	[RO]
 *				struct containing the usage lines for
 *				the FE app
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
ParamsPrintUsage(ParamUsage *param_usage)
{

	int private_args;

	/*
	 * Whether or not private args should get printed is an issue.
	 * After we get debugging standardized for each install lib, we
	 * can make print_private_args dependent on whether libspmiapp
	 * level debugging is turned on.  For now - turn it off so the
	 * private args don't get outside the group...
	 */
	int print_private_args = 0;

	/*
	 * Only print public private headers if there are private
	 * options also
	 *
	 */
	if (APP_PARAMS_COMMON_PRIVATE_USAGE || param_usage->app_private_usage)
		private_args = 1;
	else
		private_args = 0;

	(void) fprintf(stderr, APP_PARAMS_USAGE_HDR,
		param_usage->app_name);

	/* print public options */
	if (print_private_args && private_args)
		(void) fprintf(stderr, "%s", APP_PARAMS_PUBLIC_HDR);
	if (APP_PARAMS_COMMON_PUBLIC_USAGE)
		(void) fprintf(stderr, "%s", APP_PARAMS_COMMON_PUBLIC_USAGE);
	if (param_usage->app_public_usage)
		(void) fprintf(stderr, "%s", param_usage->app_public_usage);

	/* print private args */
	if (print_private_args && private_args) {
		(void) fprintf(stderr, "%s", APP_PARAMS_PRIVATE_HDR);
		if (APP_PARAMS_COMMON_PRIVATE_USAGE)
			(void) fprintf(stderr, "%s",
				(char *) APP_PARAMS_COMMON_PRIVATE_USAGE);
		if (param_usage->app_private_usage)
			(void) fprintf(stderr, "%s",
				param_usage->app_private_usage);
	}

	/* print trailing options */
	if (param_usage->app_trailing_usage)
		(void) fprintf(stderr, "%s", param_usage->app_trailing_usage);
}

/*
 * Function: ParamsGetProgramName
 * Description:
 *	Take the program name (as from argv[0]) and get it's base name
 *	and store them both in the param_usage struct for later use.
 * Scope:	PUBLIC
 * Parameters:
 *		prog - 	[RO]
 *			the argv[0] program name of this program.
 *		param_usage - [RW]
 *			struct for holding FE arg information
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
ParamsGetProgramName(char *prog, ParamUsage *param_usage)
{
	char *ptr;

	/* save the full path name */
	param_usage->app_name = xstrdup(prog);

	/* make app_name_base point at the basepath of app_name */
	if ((ptr = strrchr(prog, '/')) != NULL)
		++ptr;
	else
		ptr = prog;

	param_usage->app_name_base = xstrdup(ptr);
}

/*
 * *********************************************************************
 * This parse section is common to ttinstall and installtool.
 * If another app wants to add options not common to all apps,
 * it should follow this example. (hint, hint: like -r for pfinstall).
 * *********************************************************************
 */
unsigned int    upgradeEnabled;
/* debug should change when standardized tracing/debugging is defined */
unsigned int    debug;

/*
 * Function: ParamsParseUIArgs
 * Description:
 *	Parse the ttinstall/installtool common FE arguments.
 * Scope:	PUBLIC
 * Parameters:
 *		argc - [RO]
 *		argv - [RO]
 *		param_usage -	[RW]
 *				struct for holding FE arg information
 * Return:	[void]
 * Globals:	upgradeEnabled
 *		debug
 * Notes:
 */
void
ParamsParseUIArgs(int argc, char **argv, ParamUsage *param_usage)
{
	int c;
	char options[32];

	/*
	 * Add common BE args to getopt option string so that
	 * they will pass thru.
	 */
	(void) strcpy(options, ParamsGetCommonArgs());
	if (param_usage->app_args)
		(void) strcat(options, param_usage->app_args);

	/* parse any FE specific params */
	while ((c = getopt(argc, argv, options)) != EOF) {
		switch (c) {
		/* PUBLIC OPTIONS */

		/* PRIVATE OPTIONS */

		/*
		 * option used to allow us to easily turn on/off
		 * upgrade/server options in UI during development
		 */
		case 'u':
			upgradeEnabled = 1;
		break;
		/*
		 * v should go away and move into private BE common args
		 * as soon as standardized tracing/debugging interfaces
		 * are defined.
		 */
		case 'v':
			debug = 1;
			break;

		/* bad options or usage request */
		case '?':
			ParamsPrintUsage(param_usage);
			exit(EXIT_INSTALL_FAILURE);

		/*
		 * Do nothing if it is an 'unknown' option because we want
		 * to pass these through to the next level common getopt
		 * routine.
		 */
		default:
			break;
		}
	}
}

/*
 * Function: ParamsValidateUILastArgs
 * Description:
 *	For ttinstall/installtool: check and make sure that the right
 *	number of arguments are left after parsing all FE/BE arguments
 *	with getopt().
 * Scope:	PUBLIC
 * Parameters:
 *		argc - [RO]
 *		optindex -	[RO]
 *				index where getopt left off in the arguments
 *		param_usage -	[RW]
 *				struct for holding FE arg information
 *
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
ParamsValidateUILastArgs(int argc, int optindex, ParamUsage *param_usage)
{
	int c;

	/* insure no extraneous args on end */
	c = argc - optindex;
	if (c != 0) {
		(void) fprintf(stderr, "%s: %s",
			param_usage->app_name, APP_PARAMS_TRAILING_OPTS_ERR);
		ParamsPrintUsage(param_usage);
		exit(EXIT_INSTALL_FAILURE);
	}
}
