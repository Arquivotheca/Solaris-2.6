/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)serial_add.c	1.10	95/05/24 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "serial_impl.h"


static int
add_modem(PmtabInfo *pm, int flag)
{

	int	status;
	char	saccmd[512];


	if (is_console(pm->port)) {
	    return (SERIAL_ERR_IS_CONSOLE);
	}

	/*
	 * Check to see if the expected Service Access Controller has been
	 * set up, if not set one up or just enable it if off.
	 */

	switch (check_for_portmon(pm)) {

	case FALSE:
		/*
		 * Start a new port monitor.
		 */
		sprintf(saccmd, "set -f ; %s -a -n 2 -p %s -t ttymon -c \
/usr/lib/saf/ttymon -v %s -y %s 1>/dev/null 2>&1",
			SACADM_PATH, pm->pmtag, ADD_VERS, ADD_COMMENT);

		if(system(saccmd) != 0) {
			return (SERIAL_ERR_ADD_SACADM);
			/*NOTREACHED*/
		}

		break;

	case PMDISABLED:
		/*
		 * (re)enable the port monitor.
		 */
		sprintf(saccmd, "set -f ; %s -e -p %s", SACADM_PATH, pm->pmtag);

		if(system(saccmd) != 0) {
			return (SERIAL_ERR_ENABLE_PORTMON);
			/*NOTREACHED*/
		}
		break;

	case PMSTARTING:
		/*
		 * the port monitor is "STARTING" -- this can take several
		 * minutes.
		 */
#ifdef NOTDEF
		return (SERIAL_ERR_STARTING_PORTMON);
		/* NOTREACHED */
#endif
		/*
		 * It appears that a service _can_ be started right now.  In
		 * fact, starting a service appears to nudge the ttymon
		 * service into becoming ENABLEd -- go for it.
		 */
		break;

	case PMENABLED:
		break;

	case PMFAILED:
		/*
		 * Port monitor exists in _sactab, but has been started
		 * "count" times (see sacadm -n) and failed.  Kick start
		 * it to get things going again.
		 */
		sprintf(saccmd, "set -f ; %s -s -p %s", SACADM_PATH, pm->pmtag);

		if(system(saccmd) != 0) {
			return (SERIAL_ERR_STARTING_PORTMON);
			/*NOTREACHED*/
		}
		break;

	case SERIAL_FAILURE:
		return (SERIAL_ERR_CHECK_PORTMON);
		/*NOTREACHED*/
		break;
	}

	/*
	 * The caller has determined whether a service already existed on
	 * this port and whether we can just enable or disable the service.
	 * If there are any changes to an already running service, we must
	 * kill it and start another.
	 */
	if ((status = turn_on_port(pm, flag)) != 0) {
		return (status);
	}

	/*
	 * We need to set carrier detect to false in the prom...
	 */
	if (do_eeprom(pm->port, "false") == SERIAL_FAILURE) {
		return (SERIAL_ERR_DO_EEPROM);
		/*NOTREACHED*/
	}

	return (SERIAL_SUCCESS);
}

static int
turn_on_port(PmtabInfo *pm, int flag)
{

	int		status;
	char            pmadm_cmd[MEDBUF];
	char           ttyadm_str[128];	/* the string that ttyadm returns */
	char           pmadm_str[256];


	switch (flag) {
	case 'e':
		sprintf(pmadm_cmd, "set -f ; %s -e -p %s -s %s",
			PMADM_PATH, pm->pmtag, pm->svctag);
		(void) system(pmadm_cmd);
		break;
	case 'a':
		/*
		 * check whether the "label" for the line-speed exists in
		 * /etc/ttydefs.  If not found, return failure and make no
		 * changes.
		 */
		if (check_label(pm->ttylabel) != 0) {
			return (SERIAL_ERR_GET_STTYDEFS);
			/*NOTREACHED*/
		}

		/* run args thru ttyadm before deleting the old service */
		if ((status =
		    ttyadm_setup(pm, ttyadm_str, sizeof (ttyadm_str))) !=
		    SERIAL_SUCCESS) {
			return (status);
		}

		/* delete the old service, if any */
		if (pm->pmtag_key != NULL && pm->svctag_key != NULL) {
			sprintf(pmadm_cmd,
			    "set -f ; %s -r -p %s -s %s 1>/dev/null 2>&1",
			    PMADM_PATH, pm->pmtag_key, pm->svctag_key);
			(void) system(pmadm_cmd);
		}

		(void) pmadm_setup(pm, ttyadm_str,
		    pmadm_str, sizeof (pmadm_str));
		break;
	}

	return (SERIAL_SUCCESS);
}

/*
 * List the port monitors via "sacadm -l".
 * Look for the one our line is interested in.
 * If not found report failure by returning false.
 * Otherwise report whether the port monitor is enabled or disabled.
 */
static int
check_for_portmon(PmtabInfo *pm)
{
	char            msgbuffer[LGBUF];
	char            cmdbuf[MEDBUF];
	FILE           *cmd_results;
	int             ret_val = FALSE;

	(void) sprintf(cmdbuf, "%s -l -t ttymon", SACADM_PATH);
	if ((cmd_results = popen(cmdbuf, "r")) == NULL) {
		return (SERIAL_FAILURE);
	}
	while (fgets(msgbuffer, sizeof(msgbuffer), cmd_results)) {
		/*
		 * Check for correct portmon, make sure that it is enabled
		 */
		if (strstr(msgbuffer, pm->pmtag) != NULL) {
			if (strstr(msgbuffer, "ENABLED") != NULL)
				ret_val = PMENABLED;
			else if (strstr(msgbuffer, "DISABLED") != NULL)
				ret_val = PMDISABLED;
			else if (strstr(msgbuffer, "STARTING") != NULL)
				ret_val = PMSTARTING;
			else if (strstr(msgbuffer, "FAILED") != NULL)
				ret_val = PMFAILED;
			break;
		}	/* otherwise returns FALSE */
	}
	pclose(cmd_results);

	return (ret_val);
}

/*
 * Prepare an arg-list for an 'execv' of ttyadm.
 * Direct fork/execv avoids problems caused by 'shell' interpretation
 * of meta chars which may appear in 'prompt', etc.
 */
#define	required_push(f, a)	{*ap++ = f; *ap++ = a;}
#define	ifnotnull_push(f, a)	{if (a) {*ap++ = f; *ap++ = a;}}

static int
ttyadm_setup(PmtabInfo *pm, char *buf, int bufsiz)
{
	char		*args[64];
	const char	**ap;
	FILE		*fp;
	FILE		*errf;

	ap = (const char **)&args[0];
	*ap++ = "ttyadm";

	required_push("-d", pm->device);
	required_push("-l", pm->ttylabel);
	required_push("-s", pm->service);

	if (pm->ttyflags) {
		if (strchr(pm->ttyflags, 'b'))
			*ap++ = "-b";
		if (strchr(pm->ttyflags, 'c'))
			*ap++ = "-c";
		if (strchr(pm->ttyflags, 'I'))
			*ap++ = "-I";
	}

	ifnotnull_push("-m", pm->modules);
	ifnotnull_push("-p", pm->prompt);
	ifnotnull_push("-t", pm->timeout);
	ifnotnull_push("-S", pm->softcar);
	ifnotnull_push("-T", pm->termtype);

	*ap++ = NULL;
#ifdef DEBUG_CODE
	/* for testing, put ttyadm command in a file */
	if (debug_add_modem) {
		FILE           *f;

		f = fopen("/tmp/ttyadm.call", "w");
		if (f) {
			char          **p;

			for (p = &args[0]; *p != NULL; p++)
				fprintf(f, "<%s>\n", *p);
			fclose(f);
		}
	}
#endif
	if (fp = pipe_execv(TTYADM_PATH, args, &errf)) {
		fgets(buf, bufsiz - 1, fp);
		/* close pipe and check child's exit status */
		if (close_pipe_execl(fp)) {
			/* nonzero exit status -- read stderr */
			fgets(buf, bufsiz - 1, errf);

			return (SERIAL_ERR_TTYADM);
			/*NOTREACHED*/
		}
		fclose(errf);
#ifdef DEBUG_CODE
		/* for testing, put ttyadm result in a file */
		if (debug_add_modem) {
			FILE           *f;

			f = fopen("/tmp/ttyadm.result", "w");
			if (f) {
				fprintf(f, "<%s>\n", buf);
				fclose(f);
			}
		}
#endif
	} else {
		/* signal error calling ttyadm */
		return (SERIAL_ERR_FORK_PROC_TTYADM);
		/* NOTREACHED */
	}

	return (SERIAL_SUCCESS);
}

/*
 * Simply call ttyadm to get its version number.  Sigh.
 */
static int
ttyadm_version(char *buf, int bufsiz)
{
	char		*args[4];
	const char	**ap;
	FILE		*fp;
	FILE		*errf;

	ap = (const char **)&args[0];
	*ap++ = "ttyadm";
	*ap++ = "-V";
	*ap++ = NULL;

	if (fp = pipe_execv(TTYADM_PATH, args, &errf)) {
		fgets(buf, bufsiz - 1, fp);
		(void) close_pipe_execl(fp);
		fclose(errf);
	} else {
		/* signal error in ttyadm */
		return (SERIAL_ERR_FORK_PROC_TTYADM);
		/* NOTREACHED */
	}

	return (SERIAL_SUCCESS);
}


/*
 * Call pmadm to do the _real_ work.
 */
static int
pmadm_setup(PmtabInfo *pm, char *ttyadm_str, char *buf, int bufsiz)
{
	char		*args[64];
	const char	**ap;
	int		status;
	char		version[32];
	FILE		*fp;
	FILE		*errf;

	ap = (const char **)&args[0];
	*ap++ = "pmadm";
	*ap++ = "-a";

	required_push("-p", pm->pmtag);
	required_push("-s", pm->svctag);
	required_push("-i", pm->identity);
	status = ttyadm_version(version, sizeof (version));
	if (status != SERIAL_SUCCESS) {
		return (SERIAL_ERR_TTYADM_VERSION);
	}
	required_push("-v", version);
	ifnotnull_push("-f", pm->portflags);
	required_push("-m", ttyadm_str);
	ifnotnull_push("-y", pm->comment);

	*ap++ = NULL;
#ifdef DEBUG_CODE
	/* for testing, put pmadm call in a file */
	if (debug_add_modem) {
		FILE           *f;

		f = fopen("/tmp/pmadm.call", "w");
		if (f) {
			char          **p;

			for (p = &args[0]; *p != NULL; p++)
				fprintf(f, "<%s>\n", *p);
			fclose(f);
		}
	}
#endif
	if (fp = pipe_execv(PMADM_PATH, args, &errf)) {
		fgets(buf, bufsiz - 1, fp);
		/* close pipe and check child's exit status */
		if (close_pipe_execl(fp)) {
			/* nonzero exit status -- read stderr */
			fgets(buf, bufsiz - 1, errf);

			return (SERIAL_ERR_PMADM);
			/*NOTREACHED*/
		}
		fclose(errf);
#ifdef DEBUG_CODE
		/* for testing, put pmadm result in a file */
		if (debug_add_modem) {
			FILE           *f;

			f = fopen("/tmp/pmadm.result", "w");
			if (f) {
				fprintf(f, "<%s>\n", buf);
				fclose(f);
			}
		}
#endif
	} else {
		/* signal error in pmadm */
		return (SERIAL_ERR_FORK_PROC_PMADM);
		/* NOTREACHED */
	}

	return (SERIAL_SUCCESS);
}


static boolean_t
is_good_string(const char *string)
{

	int	i;
	int	len;


	if (string == NULL) {
		return (B_TRUE);
	}

	for (i = 0; i < (len = strlen(string)); i++) {
		if (isgraph(string[i]) == 0 ||
		    string[i] == ':' || string[i] == '\\') {
			break;
		}
	}

	if (i == len) {
		return (B_TRUE);
	}

	return (B_FALSE);
}


static boolean_t
is_good_path(const char *path)
{

	struct stat	stat_buf;


	if (path == NULL) {
		return (B_TRUE);
	}

	if (stat(path, &stat_buf) != 0) {
		return (B_FALSE);
	}

	return (B_TRUE);
}


static boolean_t
is_good_prompt(const char *prompt)
{

	int	i;
	int	len;


	if (prompt == NULL) {
		return (B_TRUE);
	}

	for (i = 0; i < (len = strlen(prompt)); i++) {
		if (isprint(prompt[i]) == 0) {
			break;
		}
		if (prompt[i] == '\\') {
			i++;
			if (prompt[i] != ':') {
				break;
			}
		}
	}

	if (i == len && prompt[i - 1] != '\\') {
		return (B_TRUE);
	}

	return (B_FALSE);
}


static boolean_t
is_good_comment(const char *string)
{

	int	i;
	int	len;


	if (string == NULL) {
		return (B_TRUE);
	}

	for (i = 0; i < (len = strlen(string)); i++) {
		if (isprint(string[i]) == 0 ||
		    string[i] == ':' || string[i] == '\\') {
			break;
		}
	}

	if (i == len) {
		return (B_TRUE);
	}

	return (B_FALSE);
}


static boolean_t
is_modem_valid(const PmtabInfo *pm, int flag)
{

	if (pm == NULL) {
		return (B_FALSE);
	}

	/* required */
	if (pm->pmtag == NULL || pm->pmtag[0] == 0 ||
	    is_good_string(pm->pmtag) == B_FALSE) {
		return (B_FALSE);
	}
	/* required */
	if (pm->svctag == NULL || pm->svctag[0] == 0 ||
	    is_good_string(pm->svctag) == B_FALSE) {
		return (B_FALSE);
	}
	/* optional */
	if (is_good_string(pm->portflags) == B_FALSE) {
		return (B_FALSE);
	}
	/* required for 'a' */
	if (flag == 'a') {
		if (pm->identity == NULL || pm->identity[0] == 0 ||
		    is_good_string(pm->identity) == B_FALSE) {
			return (B_FALSE);
		}
	} else {
		if (is_good_string(pm->identity) == B_FALSE) {
			return (B_FALSE);
		}
	}
	/* required for 'a' */
	if (flag == 'a') {
		if (pm->device == NULL || pm->device[0] == 0 ||
		    is_good_path(pm->device) == B_FALSE) {
			return (B_FALSE);
		}
	} else {
		if (is_good_path(pm->device) == B_FALSE) {
			return (B_FALSE);
		}
	}
	/* optional */
	if (is_good_string(pm->ttyflags) == B_FALSE) {
		return (B_FALSE);
	}
	/* required for 'a' */
	if (flag == 'a') {
		if (pm->service == NULL || pm->service[0] == 0 ||
		    is_good_string(pm->service) == B_FALSE) {
			return (B_FALSE);
		}
	} else {
		if (is_good_string(pm->service) == B_FALSE) {
			return (B_FALSE);
		}
	}
	/* optional */
	if (is_good_string(pm->timeout) == B_FALSE) {
		return (B_FALSE);
	}
	/* required for 'a' */
	if (flag == 'a') {
		if (pm->ttylabel == NULL || pm->ttylabel[0] == 0 ||
		    is_good_string(pm->ttylabel) == B_FALSE) {
			return (B_FALSE);
		}
	} else {
		if (is_good_string(pm->ttylabel) == B_FALSE) {
			return (B_FALSE);
		}
	}
	/* optional */
	if (is_good_string(pm->modules) == B_FALSE) {
		return (B_FALSE);
	}
	/* optional */
	if (is_good_prompt(pm->prompt) == B_FALSE) {
		return (B_FALSE);
	}
	/* optional */
	if (is_good_string(pm->termtype) == B_FALSE) {
		return (B_FALSE);
	}
	/* optional */
	if (is_good_string(pm->softcar) == B_FALSE) {
		return (B_FALSE);
	}
	/* optional */
	if (is_good_comment(pm->comment) == B_FALSE) {
		return (B_FALSE);
	}

	return (B_TRUE);
}


/*
 * The public entry points to this file follow:
 * modify_modem(), enable_modem(), disable_modem()
 */


int
modify_modem(PmtabInfo *pm)
{

	if (is_modem_valid(pm, 'a') == B_FALSE) {
		return (SERIAL_ERR_BAD_INPUT);
	}

	return (add_modem(pm, 'a'));
}


int
enable_modem(PmtabInfo *pm)
{

	if (is_modem_valid(pm, 'e') == B_FALSE) {
		return (SERIAL_ERR_BAD_INPUT);
	}

	return (add_modem(pm, 'e'));
}


int
disable_modem(PmtabInfo *pm)
{

	char            pmadm_cmd[MEDBUF];


	if (is_console(pm->port)) {
	    return (SERIAL_ERR_IS_CONSOLE);
	}

	sprintf(pmadm_cmd, "set -f ; %s -d -p %s -s %s",
	    PMADM_PATH, pm->pmtag_key, pm->svctag_key);
	return (system(pmadm_cmd));
}
