
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * Deal with the state saved by the sysIDtool programs. This
 * state includes:
 *	if the system was in a configured state or not
 *	if the bootparams request for the system's configuration
 *		succeeded.
 *	if the system is being configured on a network
 *	if the system's name service (if any) was autobound
 *	if the system's network (if any) is subnetted
 *	the TERM type
 */

#pragma	ident	"@(#)state.c	1.24	94/08/31 SMI"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "sysidtool.h"

/*
 * Local defines
 */
#define	STATE_FILE	"/etc/.sysIDtool.state"
#define	SET		'1'

/*
 * Local shared variables.
 */

static char *state_file_path = NULL;

/*
 * Function to remove last component in a pathname.
 * (Function taken from libadmutil.)
 */
static void
remove_component(char *path)
{
	char *p;

	p = strrchr(path, '/'); 		/* find last '/' 	*/
	if (p == NULL) {
		*path = '\0';			/* set path to null str	*/
	} else {
		*p = '\0';			/* zap it 		*/
	}
}

/*
 * Function to traverse a symlink path to find the real file at the end of
 * the rainbow.
 * (Function taken from libadmutil.)
 */
static int
trav_link(char **path)
{
	static char newpath[MAXPATHLEN];
	char lastpath[MAXPATHLEN];
	int len;
	char *tp;

	(void) strcpy(lastpath, *path);
	while ((len = readlink(*path, newpath, sizeof (newpath))) != -1) {
		newpath[len] = '\0';
		if (newpath[0] != '/') {
			tp = strdup(newpath);
			remove_component(lastpath);
			(void) sprintf(newpath, "%s/%s", lastpath, tp);
			free(tp);
		}
		(void) strcpy(lastpath, newpath);
		*path = newpath;
	}

	/*
	 * ENOENT or EINVAL is the normal exit case of the above loop.
	 */
	if ((errno == ENOENT) || (errno == EINVAL))
		return (0);
	else
		return (-1);
}

/*
 * Find the state file.
 */

static char *
find_state_file()
{
	char *path;

	if (state_file_path == NULL) {
		path = STATE_FILE;
		if (trav_link(&path) == 0) {
			state_file_path = strdup(path);
		} else {
			state_file_path = STATE_FILE;
		}
	}
	return (state_file_path);
}

/*
 * Read the state from the state file and return to caller.
 */
int
get_state(int *sys_configured, int *sys_bootparamed, int *sys_networked,
    int *sys_autobound, int *sys_subnetted, int *sys_passwd, int *sys_locale,
    char *term_type, int *err_num)
{
	FILE *fp;
	char tmp[MAXPATHLEN+1];
	int tlen;
	int status;

	*sys_configured = FALSE;
	*sys_bootparamed = FALSE;
	*sys_networked = FALSE;
	*sys_autobound = FALSE;
	*sys_subnetted = FALSE;
	*sys_passwd = FALSE;
	*sys_locale = FALSE;
	*term_type = NULL;
	*err_num = 0;

	if (testing) {
		status = (*sim_handle())(SIM_GET_STATE, sys_configured,
		    sys_bootparamed, sys_networked, sys_autobound,
		    sys_subnetted, sys_passwd, sys_locale, term_type,
		    err_num);
		return (status);
	}
	if ((fp = fopen(find_state_file(), "r")) == NULL) {
		fprintf(debugfp, "sysIDtool %s couldn't open: errno = %d\n",
		    find_state_file(), errno);
		*err_num = errno;
		return (FAILURE);
	}

	fprintf(debugfp, "%s opened\n", find_state_file());

	/*
	 * Read each state component.
	 */
	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state ( configured): %s", tmp);
		if (tmp[0] == SET)
			*sys_configured = TRUE;
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state (bootparamed): %s", tmp);
		if (tmp[0] == SET)
			*sys_bootparamed = TRUE;
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state (  networked): %s", tmp);
		if (tmp[0] == SET) {
			*sys_networked = TRUE;
		}
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state (  autobound): %s", tmp);
		if (tmp[0] == SET) {
			*sys_autobound = TRUE;
		}
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state (  subnetted): %s", tmp);
		if (tmp[0] == SET) {
			*sys_subnetted = TRUE;
		}
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state (     passwd): %s", tmp);
		if (tmp[0] == SET) {
			*sys_passwd = TRUE;
		}
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(tmp, MAXPATHLEN, fp) != NULL) {

		fprintf(debugfp, "state (     locale): %s", tmp);
		if (tmp[0] == SET) {
			*sys_locale = TRUE;
		}
	} else {
		(void) fclose(fp);
		return (FAILURE);
	}

	if (fgets(term_type, MAX_TERM + 1, fp) != NULL) {

		tlen = strlen(term_type);
		if ((tlen > 0) && (*((char *)(term_type + tlen - 1)) == '\n')) {
			*((char *)(term_type + tlen - 1)) = NULL;
		}
		fprintf(debugfp, "state (       term): %s\n", term_type);
	} else {
		*term_type = NULL;
		(void) fclose(fp);
		return (FAILURE);
	}

	(void) fclose(fp);

	return (SUCCESS);
}


/*
 * Write the state into the state file.
 */
void
put_state(int sys_configured, int sys_bootparamed, int sys_networked,
    int sys_autobound, int sys_subnetted, int sys_passwd, int sys_locale,
	char *term_type)
{
	mode_t cmask;	/* Current umask */
	FILE *fp;

	if (testing) {
		(void) (*sim_handle())(SIM_PUT_STATE, sys_configured,
		    sys_bootparamed, sys_networked, sys_autobound,
		    sys_subnetted, sys_passwd, sys_locale, term_type);
		return;
	}

	cmask = umask((mode_t) 022);
	fp = fopen(find_state_file(), "w");
	(void) umask(cmask);

	if (fp == NULL) {
		fprintf(debugfp, "sysIDtool %s couldn't open: errno = %d\n",
		    find_state_file(), errno);
		return;
	}

	fprintf(debugfp, "%s opened\n", find_state_file());

	/*
	 * Write each state component.
	 */
	(void) fprintf(fp, "%d\t%s\n", sys_configured,
		"# System previously configured?");

	fprintf(debugfp, "write ( configured): %d\n", sys_configured);

	(void) fprintf(fp, "%d\t%s\n", sys_bootparamed,
		"# Bootparams succeeded?");

	fprintf(debugfp, "write (bootparamed): %d\n", sys_bootparamed);

	(void) fprintf(fp, "%d\t%s\n", sys_networked,
		"# System is on a network?");

	fprintf(debugfp, "write (  networked): %d\n", sys_networked);

	(void) fprintf(fp, "%d\t%s\n", sys_autobound,
		"# Autobinder succeeded?");

	fprintf(debugfp, "write (  autobound): %d\n", sys_autobound);

	(void) fprintf(fp, "%d\t%s\n", sys_subnetted,
		"# Network has subnets?");

	fprintf(debugfp, "write (  subnetted): %d\n", sys_subnetted);

	(void) fprintf(fp, "%d\t%s\n", sys_passwd,
		"# root password prompted for?");

	fprintf(debugfp, "write (     passwd): %d\n", sys_passwd);

	(void) fprintf(fp, "%d\t%s\n", sys_locale,
		"# locale and term prompted for?");

	fprintf(debugfp, "write (     locale): %d\n", sys_locale);

	(void) fprintf(fp, "%s\n", term_type);

	fprintf(debugfp, "write (       term): %s\n", term_type);

	(void) fclose(fp);
	sync();
	sync();
}
