/*
 *  Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * sysIDtool_rootpw - system configuration tool.
 *
 * sysIDtool_rootpw is called directly from the system startup scripts.
 * Its behaviour is to check if the rootpw is already set.  If not set,
 * syIDtool prompts the user for the root pw for this system.
 */

#pragma	ident	"@(#)sysidroot.c	1.24	96/10/08 SMI"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/fcntl.h>
#include <sys/systeminfo.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpcsvc/nis.h>
#include <rpcsvc/nislib.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include "cl_database_parms.h"
#include "sysidtool.h"
#include "sysid_msgs.h"
#include "prompt.h"
#include "sysid_preconfig.h"

#define	PERIOD		'.'
#define	NIS_PLUS_DIR    "/var/nis"
#define	NIS_COLD_START	"NIS_COLD_START"
#define	ROOTKEY_FILE	"/etc/.rootkey"

/*
 * Local variables
 */

static	char	errmess[1024];

/*
 * Local routines
 */
static void keylogin(char *pw);
static	void	done(int status, int call_close);

FILE *debugfp;

/*
 * Globals referenced
 */
extern	int	errno;
int key_setnet(struct key_netstarg *);

char 	*progname;

int
main(int argc, const char *argv[])
{
	int		sys_bootparamed, sys_networked, sys_autobound;
	char		termtype[MAX_TERM+1];
	char		passwd[MAX_PASSWORD+2];
	char		e_passwd[MAX_PASSWORD+2];
	int		subnetted;
	int		syspasswd;
	int		syslocale;
	int		sys_configured;
	int		err_num;
	char		tmpstr[MAX_TERM+11];
	extern char	*optarg;
	char		*test_input = (char *)0, *test_output = (char *)0;
	int		c;
	int		do_keylogin = 1;
	int		auto_password;
	char		*password_ptr;
	char		*locale;
	int		mb_locale;
	int		i;

	/*
	 * Set the locale to the system locale, so that it is
	 * displayed localized.  However, only do this if the locale
	 * is not a multi-byte locale.  Multi-byte locales can't display
	 * in CUI mode on the console, so those just get done in English.
	 * Sysidroot only runs in CUI mode on the console.
	 */

	debugfp = open_log("sysidroot");

	locale = setlocale(LC_ALL, "");
	fprintf(debugfp, "using LC_ALL for locale\n");
	fprintf(debugfp, "locale set from setlocale is %s\n", locale);
	mb_locale = 0;
	for (i = 0; mb_locales[i]; i++) {
		if (strcmp(locale, mb_locales[i]) == 0) {
			fprintf(debugfp, "found a mb_locale\n");
			fprintf(debugfp, "found locale index is %d\n", i);
			/*
			 * the locale is one of the known multi-byte locales
			 * (these are defined in locale.c)
			 */
			mb_locale = 1;
			break;
		}
	}

	/*
	 * if the locale IS a multi-byte locale then reset the
	 * locale for sysidroot to "C"
	 */
	if (mb_locale) {
		fprintf(debugfp, "mb_locale is set\n");
		fprintf(debugfp, "resetting the locale to C\n");
		(void) setlocale(LC_ALL, "C");
	}

	auto_password = FALSE;
	password_ptr = NULL;

	/*
	 * Check to see if this system's root pw is set.
	 */

	termtype[0] = '\0';

	progname = (char *)argv[0];

	while ((c = getopt(argc, (char **) argv, "O:i:I:")) != EOF) {
		switch (c) {
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
		default:
			/* Ignore unknown options */
			break;
		}
	}

	if (testing && (test_input == (char *)0 || test_output == (char *)0)) {
		fprintf(stderr,
	    "%s: -O <name> must be accompanied by -I <name> and -i <name>",
			progname);
		exit(1);
	}

	if (testing) {
		if (sim_init(test_input, test_output) < 0) {
			fprintf(stderr,
			    "%s: Unable to initialize simulator\n",
			    progname);
			exit(1);
		}
	}

	(void) get_state(&sys_configured, &sys_bootparamed, &sys_networked,
	    &sys_autobound, &subnetted, &syspasswd, &syslocale, termtype,
	    &err_num);

	if (syspasswd == TRUE) {
		done(SUCCESS, 0);
	}

	fprintf(debugfp, "rootpw: system is not configured\n");

	/*
	 * This system is not configured. Proceed with the configuration.
	 */

	/*
	 * Set the terminal type.
	 */

	(void) sprintf(tmpstr, "TERM=%s", termtype);
	(void) putenv(tmpstr);

	/* Start the user interface */
	if (prompt_open(&argc, (char **)argv) < 0) {
		(void) fprintf(stderr, "%s: Couldn't start user interface\n",
		    argv[0]);
		exit(1);
	}


    /* 
     * Attempt to read in the sysidtool configuration
     * file and determine if there are any configuration
     * variables that are pertinent to this application
     *
     */
    if (read_config_file() == SUCCESS) {

	fprintf(debugfp, "sysidroot: Using configuration file\n");

        /* Get the encrypted root password if it has been specified */
        password_ptr = get_preconfig_value(CFG_ROOT_PASSWORD,NULL,NULL);
        if (password_ptr != NULL) {
		auto_password = TRUE;
		strcpy(e_passwd,password_ptr);
		if (set_root_password(e_passwd, errmess) != SUCCESS) {
			prompt_error(SYSID_ERR_CANT_DO_PASSWORD, 
				errmess);
		}
		do_keylogin = 0;
        }

	if (auto_password)
		fprintf(debugfp,"  encrypted root password found\n");
	}

	/*
	 * Configure the root password for the system.
	 */
	if (!auto_password) {
		prompt_password(passwd, e_passwd);
		if (passwd[0] != NULL) {
			if (set_root_password(e_passwd, errmess) != SUCCESS) {
				do_keylogin = 0;
				prompt_error(SYSID_ERR_CANT_DO_PASSWORD, errmess);
			}
		}
		sync();
	}

	/*
	 * Setup a public key entry for the host.
	 */
	if (do_keylogin && sys_networked == TRUE) {
		char	domainname[MAX_DOMAINNAME+2];
		char	ns_type[11];

		get_net_domainname(domainname);
		system_namesrv(domainname, ns_type);

		if (strcmp(ns_type, DB_VAL_NS_UFS) != 0)
			keylogin(passwd);
	}

	/*
	 * mark sysIDtool_rootpw as completed in the state file.
	 */

	put_state(TRUE, sys_bootparamed, sys_networked, sys_autobound,
		subnetted, TRUE, syslocale, termtype);

	/*
	 * Clean up the terminal interface and exit.
	 */

	done(SUCCESS, 1);
	/* NOTREACHED */
	return (SUCCESS);
}

/*
 * Set secret key on local machine
 */
static void
keylogin(char *pw)
{
	char secret[HEXKEYBYTES + 1];
	char fullname[MAXNETNAMELEN + 1];
	struct key_netstarg netst;
	int fd;

	if (getnetname(fullname) == 0) {
		fprintf(debugfp, "Could not generate netname\n");
		return;
	}

	fprintf(debugfp, "%s\n", fullname);

	if (getsecretkey(fullname, secret, pw) == 0) {
		fprintf(debugfp, "no secret key\n");
		return;
	}

	if (secret[0] == 0) {
		fprintf(debugfp, "password incorrect, trying nisplus\n");

		if (getsecretkey(fullname, secret, "nisplus") == 0 ||
		    secret[0] == 0) {
			fprintf(debugfp, "nisplus also failed\n");
			prompt_error(SYSID_ERR_CANT_DO_KEYLOGIN);
			return;
		}
	}

	fprintf(debugfp, "update /etc/.rootkey\n");

	memcpy(netst.st_priv_key, secret, HEXKEYBYTES);
	memset(secret, 0, HEXKEYBYTES);

	netst.st_pub_key[0] = 0;
	netst.st_netname = strdup(fullname);

	/* do actual key login */
	if (key_setnet(&netst) < 0)
		fprintf(debugfp, "could not set secret key\n");

	/* write unencrypted secret key into root key file */

	strcat(netst.st_priv_key, "\n");
	unlink(ROOTKEY_FILE);
	if ((fd = open(ROOTKEY_FILE, O_WRONLY+O_CREAT, 0600)) != -1) {
		write(fd, netst.st_priv_key, strlen(netst.st_priv_key)+1);
		close(fd);
		fprintf(debugfp, "wrote secret key\n");
	} else {
		fprintf(debugfp, "Could not open /etc/.rootkey\n");
	}
}

void
usage()
{
	(void) fprintf(stderr, "system config error: bad usage.\n");
}

static void
done(int status, int call_close)
{
	fprintf(debugfp, "rootpw: done\n");

	/*
	 * Clean up the curses interface.
	 */
	if (call_close)
		(void) prompt_close(GOODBYE_SYSINIT, 1);
	fprintf(debugfp, "sysidroot end\n\n");
	sync();
	exit(status);

	/*NOTREACHED*/
}
